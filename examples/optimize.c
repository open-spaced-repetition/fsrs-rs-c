#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fsrs.h"

// Default FSRS parameters (equivalent to DEFAULT_PARAMETERS in Python)
static const float DEFAULT_PARAMETERS[] = {
    0.40255, 1.18385, 3.173, 15.69105, 7.1949, 0.5345, 1.4604, 0.0046, 
    1.54575, 0.1192, 1.01925, 1.9395, 0.11, 0.29605, 2.2698, 0.2315, 
    2.9898, 0.51655, 0.6621
};
static const size_t DEFAULT_PARAMETERS_LEN = sizeof(DEFAULT_PARAMETERS) / sizeof(DEFAULT_PARAMETERS[0]);

// Structure to represent a date
typedef struct {
    int year;
    int month;
    int day;
} Date;

// Structure to represent a review history entry (date + rating)
typedef struct {
    Date date;
    int rating;
} ReviewEntry;

// Structure to represent a card's complete review history
typedef struct {
    ReviewEntry* entries;
    size_t count;
} CardHistory;

// Function to calculate days between two dates
int days_between(Date start, Date end) {
    struct tm start_tm = {0};
    struct tm end_tm = {0};
    
    start_tm.tm_year = start.year - 1900;
    start_tm.tm_mon = start.month - 1;
    start_tm.tm_mday = start.day;
    
    end_tm.tm_year = end.year - 1900;
    end_tm.tm_mon = end.month - 1;
    end_tm.tm_mday = end.day;
    
    time_t start_time = mktime(&start_tm);
    time_t end_time = mktime(&end_tm);
    
    return (int)((end_time - start_time) / (24 * 60 * 60));
}

// Function to create sample review histories for cards
CardHistory* create_review_histories_for_cards(size_t* num_cards) {
    // Sample review histories (equivalent to the Python data)
    ReviewEntry sample_histories[][10] = {
        // Card 1
        {{{2023, 1, 1}, 3}, {{2023, 1, 2}, 4}, {{2023, 1, 5}, 3}, 
         {{2023, 1, 15}, 4}, {{2023, 2, 1}, 3}, {{2023, 2, 20}, 4}},
        
        // Card 2
        {{{2023, 1, 1}, 2}, {{2023, 1, 2}, 3}, {{2023, 1, 4}, 4}, 
         {{2023, 1, 12}, 3}, {{2023, 1, 28}, 4}, {{2023, 2, 15}, 3}, {{2023, 3, 5}, 4}},
        
        // Card 3
        {{{2023, 1, 1}, 4}, {{2023, 1, 8}, 4}, {{2023, 1, 24}, 3}, 
         {{2023, 2, 10}, 4}, {{2023, 3, 1}, 3}},
        
        // Card 4
        {{{2023, 1, 1}, 1}, {{2023, 1, 2}, 1}, {{2023, 1, 3}, 3}, 
         {{2023, 1, 6}, 4}, {{2023, 1, 16}, 4}, {{2023, 2, 1}, 3}, {{2023, 2, 20}, 4}},
        
        // Card 5
        {{{2023, 1, 1}, 3}, {{2023, 1, 3}, 3}, {{2023, 1, 8}, 2}, 
         {{2023, 1, 10}, 4}, {{2023, 1, 22}, 3}, {{2023, 2, 5}, 4}, {{2023, 2, 25}, 3}},
        
        // Card 6
        {{{2023, 1, 1}, 4}, {{2023, 1, 9}, 3}, {{2023, 1, 19}, 4}, 
         {{2023, 2, 5}, 3}, {{2023, 2, 25}, 4}},
        
        // Card 7
        {{{2023, 1, 1}, 2}, {{2023, 1, 2}, 3}, {{2023, 1, 5}, 4}, 
         {{2023, 1, 15}, 3}, {{2023, 1, 30}, 4}, {{2023, 2, 15}, 3}, {{2023, 3, 5}, 4}},
        
        // Card 8
        {{{2023, 1, 1}, 3}, {{2023, 1, 4}, 4}, {{2023, 1, 14}, 4}, 
         {{2023, 2, 1}, 3}, {{2023, 2, 20}, 4}},
        
        // Card 9
        {{{2023, 1, 1}, 1}, {{2023, 1, 1}, 3}, {{2023, 1, 2}, 1}, 
         {{2023, 1, 2}, 3}, {{2023, 1, 3}, 3}, {{2023, 1, 7}, 3}, 
         {{2023, 1, 15}, 4}, {{2023, 1, 31}, 3}, {{2023, 2, 15}, 4}, {{2023, 3, 5}, 3}},
        
        // Card 10
        {{{2023, 1, 1}, 4}, {{2023, 1, 10}, 3}, {{2023, 1, 20}, 4}, 
         {{2023, 2, 5}, 4}, {{2023, 2, 25}, 3}, {{2023, 3, 15}, 4}},
        
        // Card 11
        {{{2023, 1, 1}, 1}, {{2023, 1, 2}, 2}, {{2023, 1, 3}, 3}, 
         {{2023, 1, 4}, 4}, {{2023, 1, 10}, 3}, {{2023, 1, 20}, 4}, 
         {{2023, 2, 5}, 3}, {{2023, 2, 25}, 4}},
        
        // Card 12
        {{{2023, 1, 1}, 3}, {{2023, 1, 5}, 4}, {{2023, 1, 15}, 3}, 
         {{2023, 1, 30}, 4}, {{2023, 2, 15}, 3}, {{2023, 3, 5}, 4}},
        
        // Card 13
        {{{2023, 1, 1}, 2}, {{2023, 1, 3}, 3}, {{2023, 1, 7}, 4}, 
         {{2023, 1, 17}, 3}, {{2023, 2, 1}, 4}, {{2023, 2, 20}, 3}, {{2023, 3, 10}, 4}},
        
        // Card 14
        {{{2023, 1, 1}, 4}, {{2023, 1, 12}, 3}, {{2023, 1, 25}, 4}, 
         {{2023, 2, 10}, 3}, {{2023, 3, 1}, 4}}
    };
    
    size_t sample_sizes[] = {6, 7, 5, 7, 7, 5, 7, 5, 10, 6, 8, 6, 7, 5};
    size_t num_sample_histories = sizeof(sample_sizes) / sizeof(sample_sizes[0]);
    
    // Create 100 cards by cycling through the sample data
    *num_cards = 100;
    CardHistory* histories = malloc(*num_cards * sizeof(CardHistory));
    
    for (size_t i = 0; i < *num_cards; i++) {
        size_t sample_idx = i % num_sample_histories;
        size_t entry_count = sample_sizes[sample_idx];
        
        histories[i].count = entry_count;
        histories[i].entries = malloc(entry_count * sizeof(ReviewEntry));
        
        for (size_t j = 0; j < entry_count; j++) {
            histories[i].entries[j] = sample_histories[sample_idx][j];
        }
    }
    
    return histories;
}

// Function to convert a card history to FSRSItems
fsrs_FSRSItem** convert_to_fsrs_items(CardHistory* history, size_t* item_count) {
    if (history->count == 0) {
        *item_count = 0;
        return NULL;
    }
    
    // Create a dynamic array to store item pointers
    fsrs_FSRSItem** items = malloc(history->count * sizeof(fsrs_FSRSItem*));
    size_t valid_items = 0;
    
    // Track the last review date across the entire history
    Date last_date = history->entries[0].date;
    
    for (size_t i = 0; i < history->count; i++) {
        // Create reviews array for this item (includes all reviews up to this point)
        fsrs_FSRSReview* reviews = malloc((i + 1) * sizeof(fsrs_FSRSReview));
        
        // Reset last_date for this FSRSItem
        Date item_last_date = history->entries[0].date;
        bool has_positive_delta_t = false;
        
        for (size_t j = 0; j <= i; j++) {
            int delta_t;
            if (j == 0) {
                delta_t = 0;  // First review always has delta_t = 0
            } else {
                delta_t = days_between(item_last_date, history->entries[j].date);
                if (delta_t > 0) {
                    has_positive_delta_t = true;
                }
            }
            reviews[j].rating = history->entries[j].rating;
            reviews[j].delta_t = delta_t;
            item_last_date = history->entries[j].date;  // Update last_date for next delta_t calculation
        }
        
        // Create FsrsReviews structure
        fsrs_FsrsReviews review_history = {reviews, i + 1};

        // Create FSRSItem
        fsrs_FSRSItem* item = fsrs_item_new(&review_history);

        // Only add items that have more than one review AND at least one review with delta_t > 0
        if (i > 0 && has_positive_delta_t) {
            items[valid_items] = item;
            valid_items++;
        } else {
            fsrs_item_free(item);
        }
        
        free(reviews);
    }
    
    *item_count = valid_items;
    return items;
}

// Function to collect all FSRSItems from all card histories
fsrs_FSRSItem** collect_all_fsrs_items(CardHistory* histories, size_t num_cards, size_t* total_items) {
    // First pass: count total items
    size_t total_count = 0;
    for (size_t i = 0; i < num_cards; i++) {
        size_t item_count;
        fsrs_FSRSItem** card_items = convert_to_fsrs_items(&histories[i], &item_count);
        total_count += item_count;
        
        // Free the temporary items for counting
        for (size_t j = 0; j < item_count; j++) {
            fsrs_item_free(card_items[j]);
        }
        free(card_items);
    }
    
    if (total_count == 0) {
        *total_items = 0;
        return NULL;
    }
    
    // Allocate array for all item pointers
    fsrs_FSRSItem** all_items = malloc(total_count * sizeof(fsrs_FSRSItem*));
    size_t current_index = 0;
    
    // Second pass: collect all items
    for (size_t i = 0; i < num_cards; i++) {
        size_t item_count;
        fsrs_FSRSItem** card_items = convert_to_fsrs_items(&histories[i], &item_count);
        
        for (size_t j = 0; j < item_count; j++) {
            all_items[current_index] = card_items[j];
            current_index++;
        }
        
        free(card_items);  // Free the temporary array, but not the items themselves
    }
    
    *total_items = current_index;
    return all_items;
}

// Function to print parameters
void print_parameters(const float* params, size_t len, const char* name) {
    printf("%s = [", name);
    for (size_t i = 0; i < len; i++) {
        printf("%.5f", params[i]);
        if (i < len - 1) printf(", ");
    }
    printf("]\n");
}

// Function to free card histories
void free_card_histories(CardHistory* histories, size_t num_cards) {
    for (size_t i = 0; i < num_cards; i++) {
        free(histories[i].entries);
    }
    free(histories);
}

// Function to free FSRSItems array
void free_fsrs_items(fsrs_FSRSItem** items, size_t count) {
    for (size_t i = 0; i < count; i++) {
        fsrs_item_free(items[i]);
    }
    free(items);
}

// Helper function to create a contiguous array from array of pointers
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

int main() {
    printf("FSRS Parameter Optimization Example\n");
    printf("===================================\n\n");
    
    // Create review histories for cards
    size_t num_cards;
    CardHistory* review_histories = create_review_histories_for_cards(&num_cards);
    printf("Created review histories for %zu cards\n", num_cards);
    
    // Convert review histories to FSRSItems
    size_t total_items;
    fsrs_FSRSItem** fsrs_items = collect_all_fsrs_items(review_histories, num_cards, &total_items);
    printf("Total FSRSItems: %zu\n", total_items);
    
    // Create an FSRS instance with default parameters
    const fsrs_FSRS* fsrs = fsrs_new(DEFAULT_PARAMETERS, DEFAULT_PARAMETERS_LEN);
    print_parameters(DEFAULT_PARAMETERS, DEFAULT_PARAMETERS_LEN, "DEFAULT_PARAMETERS");
    
    // Create contiguous array for the train_set
    fsrs_FSRSItem* contiguous_items = create_contiguous_items(fsrs_items, total_items);
    
    // Create FsrsItems structure for optimization
    fsrs_FsrsItems train_set = {contiguous_items, total_items};

    // Optimize the FSRS model using the created items
    printf("\nOptimizing parameters...\n");
    float* optimized_parameters = fsrs_compute_parameters(fsrs, &train_set);
    
    if (optimized_parameters != NULL) {
        print_parameters(optimized_parameters, DEFAULT_PARAMETERS_LEN, "OPTIMIZED_PARAMETERS");
        
        // Clean up optimized parameters
        fsrs_parameters_free(optimized_parameters);
    } else {
        printf("Parameter optimization failed!\n");
    }
    
    // Clean up
    free(contiguous_items);  // Free the contiguous array
    free_fsrs_items(fsrs_items, total_items);
    free_card_histories(review_histories, num_cards);
    fsrs_free(fsrs);
    
    printf("\nOptimization complete!\n");
    return 0;
}

