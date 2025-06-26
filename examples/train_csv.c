#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fsrs.h"

// Default FSRS parameters
static const float DEFAULT_PARAMETERS[] = {
    0.40255, 1.18385, 3.173, 15.69105, 7.1949, 0.5345, 1.4604, 0.0046, 
    1.54575, 0.1192, 1.01925, 1.9395, 0.11, 0.29605, 2.2698, 0.2315, 
    2.9898, 0.51655, 0.6621
};
static const size_t DEFAULT_PARAMETERS_LEN = sizeof(DEFAULT_PARAMETERS) / sizeof(DEFAULT_PARAMETERS[0]);

// Structure to represent a datetime with timezone offset
typedef struct {
    struct tm tm;
    time_t timestamp;
} DateTime;

// Structure to represent a review entry from CSV
typedef struct {
    char* card_id;
    DateTime review_time;
    int rating;
    int state;
} CSVRecord;

// Structure to represent a card's review history
typedef struct {
    char* card_id;
    CSVRecord* entries;
    size_t count;
    size_t capacity;
} CardHistory;

// Structure to represent collection of card histories
typedef struct {
    CardHistory* cards;
    size_t count;
    size_t capacity;
} CardHistoryCollection;

// Function to download file using curl (simulating urllib.request.urlretrieve)
int download_file(const char* url, const char* filename) {
    char command[1024];
    snprintf(command, sizeof(command), "curl -L -o %s %s", filename, url);
    return system(command);
}

// Function to check if file exists
int file_exists(const char* filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

// Function to parse timestamp from milliseconds to DateTime with timezone adjustments
DateTime parse_timestamp(long long timestamp_ms) {
    DateTime dt;
    
    // Convert milliseconds to seconds
    time_t timestamp_sec = timestamp_ms / 1000;
    
    // Convert to UTC
    struct tm* utc_tm = gmtime(&timestamp_sec);
    dt.tm = *utc_tm;
    dt.timestamp = timestamp_sec;
    
    // Add 8 hours for UTC+8
    dt.timestamp += 8 * 3600;
    
    // Subtract 4 hours for next day cutoff
    dt.timestamp -= 4 * 3600;
    
    // Update tm structure
    struct tm* adjusted_tm = gmtime(&dt.timestamp);
    dt.tm = *adjusted_tm;
    
    return dt;
}

// Function to calculate days difference between two DateTimes
int date_diff_in_days(DateTime a, DateTime b) {
    // Set time to midnight for both dates
    struct tm a_date = a.tm;
    struct tm b_date = b.tm;
    
    a_date.tm_hour = 0;
    a_date.tm_min = 0;
    a_date.tm_sec = 0;
    
    b_date.tm_hour = 0;
    b_date.tm_min = 0;
    b_date.tm_sec = 0;
    
    time_t a_time = mktime(&a_date);
    time_t b_time = mktime(&b_date);
    
    return (int)((b_time - a_time) / (24 * 3600));
}

// Function to trim whitespace from a string
char* trim_whitespace(char* str) {
    char* end;
    
    // Trim leading space
    while(isspace((unsigned char)*str)) str++;
    
    if(*str == 0) return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    end[1] = '\0';
    
    return str;
}

// Function to parse CSV line
CSVRecord parse_csv_line(char* line) {
    CSVRecord record = {0};
    
    char* token = strtok(line, ",");
    int field = 0;
    
    while (token != NULL && field < 4) {
        token = trim_whitespace(token);
        
        switch (field) {
            case 0: // card_id
                record.card_id = malloc(strlen(token) + 1);
                strcpy(record.card_id, token);
                break;
            case 1: // review_time (timestamp in milliseconds)
                record.review_time = parse_timestamp(atoll(token));
                break;
            case 2: // review_rating
                record.rating = atoi(token);
                break;
            case 3: // review_state
                record.state = atoi(token);
                break;
        }
        
        token = strtok(NULL, ",");
        field++;
    }
    
    return record;
}

// Function to read CSV file and parse records
CSVRecord* read_csv_file(const char* filename, size_t* record_count) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        *record_count = 0;
        return NULL;
    }
    
    CSVRecord* records = NULL;
    size_t capacity = 1000;
    size_t count = 0;
    char line[1024];
    
    records = malloc(capacity * sizeof(CSVRecord));
    
    // Skip header line
    if (fgets(line, sizeof(line), file) == NULL) {
        printf("Error: Empty file or could not read header\n");
        fclose(file);
        free(records);
        *record_count = 0;
        return NULL;
    }
    
    // Read data lines
    while (fgets(line, sizeof(line), file)) {
        if (count >= capacity) {
            capacity *= 2;
            records = realloc(records, capacity * sizeof(CSVRecord));
        }
        
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) > 0) {
            records[count] = parse_csv_line(line);
            count++;
        }
    }
    
    fclose(file);
    *record_count = count;
    return records;
}

// Function to group reviews by card_id
CardHistoryCollection group_reviews_by_card(CSVRecord* records, size_t record_count) {
    CardHistoryCollection collection = {0};
    collection.capacity = 100;
    collection.cards = malloc(collection.capacity * sizeof(CardHistory));
    
    for (size_t i = 0; i < record_count; i++) {
        CSVRecord* record = &records[i];
        
        // Find existing card or create new one
        CardHistory* card = NULL;
        for (size_t j = 0; j < collection.count; j++) {
            if (strcmp(collection.cards[j].card_id, record->card_id) == 0) {
                card = &collection.cards[j];
                break;
            }
        }
        
        if (!card) {
            // Create new card
            if (collection.count >= collection.capacity) {
                collection.capacity *= 2;
                collection.cards = realloc(collection.cards, collection.capacity * sizeof(CardHistory));
            }
            
            card = &collection.cards[collection.count];
            card->card_id = malloc(strlen(record->card_id) + 1);
            strcpy(card->card_id, record->card_id);
            card->entries = malloc(10 * sizeof(CSVRecord));
            card->count = 0;
            card->capacity = 10;
            collection.count++;
        }
        
        // Add entry to card
        if (card->count >= card->capacity) {
            card->capacity *= 2;
            card->entries = realloc(card->entries, card->capacity * sizeof(CSVRecord));
        }
        
        card->entries[card->count] = *record;
        card->count++;
    }
    
    // Sort reviews for each card by time
    for (size_t i = 0; i < collection.count; i++) {
        CardHistory* card = &collection.cards[i];
        
        // Simple bubble sort by timestamp
        for (size_t j = 0; j < card->count - 1; j++) {
            for (size_t k = 0; k < card->count - j - 1; k++) {
                if (card->entries[k].review_time.timestamp > card->entries[k + 1].review_time.timestamp) {
                    CSVRecord temp = card->entries[k];
                    card->entries[k] = card->entries[k + 1];
                    card->entries[k + 1] = temp;
                }
            }
        }
    }
    
    return collection;
}

// Function to check if a review state is learning state
int is_learning_state(int state) {
    return state == 0 || state == 1; // New = 0, Learning = 1
}

// Function to remove revlog before last learning
void remove_revlog_before_last_learning(CardHistory* card) {
    if (card->count == 0) return;
    
    int last_learning_block_start = -1;
    
    // Find the start of the last learning block by scanning backwards
    for (int i = card->count - 1; i >= 0; i--) {
        if (is_learning_state(card->entries[i].state)) {
            last_learning_block_start = i;
        } else if (last_learning_block_start != -1) {
            break;
        }
    }
    
    // Remove entries before last learning block
    if (last_learning_block_start > 0) {
        size_t new_count = card->count - last_learning_block_start;
        memmove(card->entries, &card->entries[last_learning_block_start], new_count * sizeof(CSVRecord));
        card->count = new_count;
    } else if (last_learning_block_start == -1) {
        // No learning found, remove all entries
        card->count = 0;
    }
}

// Function to convert card history to FSRS items
fsrs_FSRSItem** convert_to_fsrs_items(CardHistory* card, size_t* item_count) {
    *item_count = 0;
    
    if (card->count == 0) {
        return NULL;
    }
    
    fsrs_FSRSItem** items = malloc(card->count * sizeof(fsrs_FSRSItem*));
    DateTime last_date = card->entries[0].review_time;
    
    for (size_t i = 0; i < card->count; i++) {
        // Create reviews array for this item (includes all reviews up to this point)
        fsrs_FSRSReview* reviews = malloc((i + 1) * sizeof(fsrs_FSRSReview));
        DateTime item_last_date = card->entries[0].review_time;
        int has_positive_delta_t = 0;
        
        for (size_t j = 0; j <= i; j++) {
            int delta_t;
            if (j == 0) {
                delta_t = 0;
            } else {
                delta_t = date_diff_in_days(item_last_date, card->entries[j].review_time);
                if (delta_t > 0) {
                    has_positive_delta_t = 1;
                }
            }
            
            reviews[j].rating = card->entries[j].rating;
            reviews[j].delta_t = delta_t;
            item_last_date = card->entries[j].review_time;
        }
        
        // Create FsrsReviews structure
        fsrs_FsrsReviews review_history = {reviews, i + 1};
        
        // Create FSRSItem
        fsrs_FSRSItem* item = fsrs_item_new(&review_history);
        
        // Only add items that have delta_t > 0 (not same day reviews)
        if (has_positive_delta_t) {
            items[*item_count] = item;
            (*item_count)++;
        } else {
            fsrs_item_free(item);
        }
        
        free(reviews);
        last_date = card->entries[i].review_time;
    }
    
    return items;
}

// Function to collect all FSRS items from all cards
fsrs_FSRSItem** collect_all_fsrs_items(CardHistoryCollection* collection, size_t* total_items) {
    *total_items = 0;
    
    // First pass: count total items
    size_t total_count = 0;
    for (size_t i = 0; i < collection->count; i++) {
        size_t item_count;
        fsrs_FSRSItem** card_items = convert_to_fsrs_items(&collection->cards[i], &item_count);
        total_count += item_count;
        
        // Free temporary items
        for (size_t j = 0; j < item_count; j++) {
            fsrs_item_free(card_items[j]);
        }
        free(card_items);
    }
    
    if (total_count == 0) {
        return NULL;
    }
    
    // Allocate array for all items
    fsrs_FSRSItem** all_items = malloc(total_count * sizeof(fsrs_FSRSItem*));
    size_t current_index = 0;
    
    // Second pass: collect all items
    for (size_t i = 0; i < collection->count; i++) {
        size_t item_count;
        fsrs_FSRSItem** card_items = convert_to_fsrs_items(&collection->cards[i], &item_count);
        
        for (size_t j = 0; j < item_count; j++) {
            all_items[current_index] = card_items[j];
            current_index++;
        }
        
        free(card_items);
    }
    
    *total_items = current_index;
    return all_items;
}

// Function to create contiguous array from array of pointers
fsrs_FSRSItem* create_contiguous_items(fsrs_FSRSItem** item_pointers, size_t count) {
    if (count == 0 || item_pointers == NULL) {
        return NULL;
    }
    
    fsrs_FSRSItem* contiguous_items = malloc(count * sizeof(fsrs_FSRSItem));
    for (size_t i = 0; i < count; i++) {
        contiguous_items[i] = *item_pointers[i];
    }
    
    return contiguous_items;
}

// Function to print parameters
void print_parameters(const float* params, size_t len, const char* name) {
    printf("%s: [", name);
    for (size_t i = 0; i < len; i++) {
        printf("%.5f", params[i]);
        if (i < len - 1) printf(", ");
    }
    printf("]\n");
}

// Function to free CSV records
void free_csv_records(CSVRecord* records, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free(records[i].card_id);
    }
    free(records);
}

// Function to free card history collection
void free_card_history_collection(CardHistoryCollection* collection) {
    for (size_t i = 0; i < collection->count; i++) {
        free(collection->cards[i].card_id);
        free(collection->cards[i].entries);
    }
    free(collection->cards);
}

// Function to free FSRS items
void free_fsrs_items(fsrs_FSRSItem** items, size_t count) {
    for (size_t i = 0; i < count; i++) {
        fsrs_item_free(items[i]);
    }
    free(items);
}

int main() {
    printf("FSRS CSV Training\n");
    printf("=================\n\n");
    
    clock_t start_time = clock();
    
    // Check if revlog.csv exists, download if not
    const char* csv_filename = "revlog.csv";
    if (!file_exists(csv_filename)) {
        printf("Downloading revlog.csv...\n");
        const char* url = "https://github.com/open-spaced-repetition/fsrs-rs/files/15046782/revlog.csv";
        if (download_file(url, csv_filename) != 0) {
            printf("Error: Failed to download CSV file\n");
            return 1;
        }
        printf("Download complete.\n\n");
    }
    
    // Read CSV file
    size_t record_count;
    CSVRecord* records = read_csv_file(csv_filename, &record_count);
    if (!records) {
        printf("Error: Failed to read CSV file\n");
        return 1;
    }
    
    printf("Read %zu records from CSV\n", record_count);
    
    // Group reviews by card
    CardHistoryCollection collection = group_reviews_by_card(records, record_count);
    printf("Grouped into %zu cards\n", collection.count);
    
    // Remove revlog before last learning for each card
    size_t cards_with_reviews = 0;
    for (size_t i = 0; i < collection.count; i++) {
        remove_revlog_before_last_learning(&collection.cards[i]);
        if (collection.cards[i].count > 0) {
            cards_with_reviews++;
        }
    }
    printf("Cards with reviews after filtering: %zu\n", cards_with_reviews);
    
    // Convert to FSRS items
    size_t total_items;
    fsrs_FSRSItem** fsrs_items = collect_all_fsrs_items(&collection, &total_items);
    printf("Total FSRS items: %zu\n", total_items);
    
    if (total_items == 0) {
        printf("No valid FSRS items found for training\n");
        free_csv_records(records, record_count);
        free_card_history_collection(&collection);
        return 1;
    }
    
    // Create FSRS instance with empty parameters for optimization
    const fsrs_FSRS* fsrs = fsrs_new(NULL, 0);
    
    // Create contiguous array for training
    fsrs_FSRSItem* contiguous_items = create_contiguous_items(fsrs_items, total_items);
    fsrs_FsrsItems train_set = {contiguous_items, total_items};
    
    // Optimize parameters
    printf("\nOptimizing parameters...\n");
    float* optimized_parameters = fsrs_compute_parameters(fsrs, &train_set);
    
    if (optimized_parameters != NULL) {
        print_parameters(optimized_parameters, DEFAULT_PARAMETERS_LEN, "optimized parameters");
        fsrs_parameters_free(optimized_parameters);
    } else {
        printf("Parameter optimization failed!\n");
    }
    
    clock_t end_time = clock();
    double training_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Full training time: %.2fs\n", training_time);
    
    // Cleanup
    free(contiguous_items);
    free_fsrs_items(fsrs_items, total_items);
    free_card_history_collection(&collection);
    free_csv_records(records, record_count);
    fsrs_free(fsrs);
    
    return 0;
}
