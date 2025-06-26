#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <tgmath.h>
#include "fsrs.h"

// Default FSRS parameters (equivalent to DEFAULT_PARAMETERS in Python)
static const float DEFAULT_PARAMETERS[] = {
    0.40255f, 1.18385f, 3.173f, 15.69105f, 7.1949f, 0.5345f, 1.4604f, 0.0046f, 
    1.54575f, 0.1192f, 1.01925f, 1.9395f, 0.11f, 0.29605f, 2.2698f, 0.2315f, 
    2.9898f, 0.51655f, 0.6621f
};
static const size_t DEFAULT_PARAMETERS_LEN = sizeof(DEFAULT_PARAMETERS) / sizeof(DEFAULT_PARAMETERS[0]);

// Structure to represent a date
typedef struct {
    int32_t year;
    int32_t month;
    int32_t day;
} Date;

// Structure to represent a review history entry (date + rating)
typedef struct {
    Date date;
    int32_t rating;
} ReviewEntry;

// Structure to represent a card's complete review history
typedef struct {
    ReviewEntry* entries;
    size_t count;
} CardHistory;

// Function to calculate days between two dates
static int32_t days_between(const Date start, const Date end) {
    struct tm start_tm = {0};
    struct tm end_tm = {0};
    
    start_tm.tm_year = start.year - 1900;
    start_tm.tm_mon = start.month - 1;
    start_tm.tm_mday = start.day;
    
    end_tm.tm_year = end.year - 1900;
    end_tm.tm_mon = end.month - 1;
    end_tm.tm_mday = end.day;
    
    const time_t start_time = mktime(&start_tm);
    const time_t end_time = mktime(&end_tm);
    
    if (start_time == -1 || end_time == -1) {
        return -1; // Error in date conversion
    }
    
    return (int32_t)((end_time - start_time) / (24 * 60 * 60));
}

// Function to create sample review histories for cards
static CardHistory* create_review_histories_for_cards(size_t* const num_cards) {
    // Sample review histories (equivalent to the Python data)
    static const ReviewEntry sample_histories[][10] = {
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
    
    static const size_t sample_sizes[] = {6, 7, 5, 7, 7, 5, 7, 5, 10, 6, 8, 6, 7, 5};
    static const size_t num_sample_histories = sizeof(sample_sizes) / sizeof(sample_sizes[0]);
    
    // Create 100 cards by cycling through the sample data
    *num_cards = 100;
    CardHistory* const histories = malloc(*num_cards * sizeof(CardHistory));
    if (!histories) {
        *num_cards = 0;
        return NULL;
    }
    
    for (size_t i = 0; i < *num_cards; i++) {
        const size_t sample_idx = i % num_sample_histories;
        const size_t entry_count = sample_sizes[sample_idx];
        
        histories[i].count = entry_count;
        histories[i].entries = malloc(entry_count * sizeof(ReviewEntry));
        
        if (!histories[i].entries) {
            // Cleanup on failure
            for (size_t j = 0; j < i; j++) {
                free(histories[j].entries);
            }
            free(histories);
            *num_cards = 0;
            return NULL;
        }
        
        memcpy(histories[i].entries, sample_histories[sample_idx], 
               entry_count * sizeof(ReviewEntry));
    }
    
    return histories;
}

// Function to convert a card history to FSRSItems
static fsrs_FSRSItem** convert_to_fsrs_items(const CardHistory* const history, size_t* const item_count) {
    if (!history || history->count == 0) {
        *item_count = 0;
        return NULL;
    }
    
    // Create a dynamic array to store item pointers
    fsrs_FSRSItem** const items = malloc(history->count * sizeof(fsrs_FSRSItem*));
    if (!items) {
        *item_count = 0;
        return NULL;
    }
    
    size_t valid_items = 0;
    
    for (size_t i = 0; i < history->count; i++) {
        // Create reviews array for this item (includes all reviews up to this point)
        fsrs_FSRSReview* const reviews = malloc((i + 1) * sizeof(fsrs_FSRSReview));
        if (!reviews) {
            // Cleanup on failure
            for (size_t j = 0; j < valid_items; j++) {
                fsrs_item_free(items[j]);
            }
            free(items);
            *item_count = 0;
            return NULL;
        }
        
        // Reset last_date for this FSRSItem
        Date item_last_date = history->entries[0].date;
        bool has_positive_delta_t = false;
        
        for (size_t j = 0; j <= i; j++) {
            int32_t delta_t;
            if (j == 0) {
                delta_t = 0;  // First review always has delta_t = 0
            } else {
                delta_t = days_between(item_last_date, history->entries[j].date);
                if (delta_t > 0) {
                    has_positive_delta_t = true;
                }
            }
            reviews[j].rating = (uint32_t)history->entries[j].rating;
            reviews[j].delta_t = (uint32_t)fmax(0, delta_t);
            item_last_date = history->entries[j].date;  // Update last_date for next delta_t calculation
        }
        
        // Create FsrsReviews structure
        fsrs_FsrsReviews review_history = {
            .reviews = reviews, 
            .len = i + 1
        };

        // Create FSRSItem
        fsrs_FSRSItem* const item = fsrs_item_new(&review_history);

        // Only add items that have more than one review AND at least one review with delta_t > 0
        if (i > 0 && has_positive_delta_t && item) {
            items[valid_items] = item;
            valid_items++;
        } else if (item) {
            fsrs_item_free(item);
        }
        
        free(reviews);
    }
    
    *item_count = valid_items;
    return items;
}

// Function to collect all FSRSItems from all card histories
static fsrs_FSRSItem** collect_all_fsrs_items(const CardHistory* const histories, 
                                              const size_t num_cards, 
                                              size_t* const total_items) {
    // First pass: count total items
    size_t total_count = 0;
    for (size_t i = 0; i < num_cards; i++) {
        size_t item_count;
        fsrs_FSRSItem** const card_items = convert_to_fsrs_items(&histories[i], &item_count);
        total_count += item_count;
        
        // Free the temporary items for counting
        if (card_items) {
            for (size_t j = 0; j < item_count; j++) {
                fsrs_item_free(card_items[j]);
            }
            free(card_items);
        }
    }
    
    if (total_count == 0) {
        *total_items = 0;
        return NULL;
    }
    
    // Allocate array for all item pointers
    fsrs_FSRSItem** const all_items = malloc(total_count * sizeof(fsrs_FSRSItem*));
    if (!all_items) {
        *total_items = 0;
        return NULL;
    }
    
    size_t current_index = 0;
    
    // Second pass: collect all items
    for (size_t i = 0; i < num_cards; i++) {
        size_t item_count;
        fsrs_FSRSItem** const card_items = convert_to_fsrs_items(&histories[i], &item_count);
        
        if (card_items) {
            for (size_t j = 0; j < item_count; j++) {
                all_items[current_index] = card_items[j];
                current_index++;
            }
            
            free(card_items);  // Free the temporary array, but not the items themselves
        }
    }
    
    *total_items = current_index;
    return all_items;
}

// Function to print parameters
static void print_parameters(const float* const params, const size_t len, const char* const name) {
    if (!params || !name) {
        return;
    }
    
    printf("%s = [", name);
    for (size_t i = 0; i < len; i++) {
        printf("%.5f", params[i]);
        if (i < len - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}

// Function to free card histories
static void free_card_histories(CardHistory* const histories, const size_t num_cards) {
    if (!histories) {
        return;
    }
    
    for (size_t i = 0; i < num_cards; i++) {
        free(histories[i].entries);
    }
    free(histories);
}

// Function to free FSRSItems array
static void free_fsrs_items(fsrs_FSRSItem** const items, const size_t count) {
    if (!items) {
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        fsrs_item_free(items[i]);
    }
    free(items);
}

// Helper function to create a contiguous array from array of pointers
static fsrs_FSRSItem* create_contiguous_items(fsrs_FSRSItem* const* const item_pointers, 
                                             const size_t count) {
    if (count == 0 || !item_pointers) {
        return NULL;
    }
    
    fsrs_FSRSItem* const contiguous_items = malloc(count * sizeof(fsrs_FSRSItem));
    if (!contiguous_items) {
        return NULL;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (item_pointers[i]) {
            contiguous_items[i] = *item_pointers[i];
        }
    }
    
    return contiguous_items;
}

int32_t main(void) {
    printf("FSRS Parameter Optimization Example\n");
    printf("===================================\n\n");
    
    // Create review histories for cards
    size_t num_cards;
    CardHistory* const review_histories = create_review_histories_for_cards(&num_cards);
    if (!review_histories) {
        fprintf(stderr, "Error: Failed to create review histories\n");
        return EXIT_FAILURE;
    }
    printf("Created review histories for %zu cards\n", num_cards);
    
    // Convert review histories to FSRSItems
    size_t total_items;
    fsrs_FSRSItem** const fsrs_items = collect_all_fsrs_items(review_histories, num_cards, &total_items);
    if (!fsrs_items) {
        fprintf(stderr, "Error: Failed to collect FSRS items\n");
        free_card_histories(review_histories, num_cards);
        return EXIT_FAILURE;
    }
    printf("Total FSRSItems: %zu\n", total_items);
    
    // Create an FSRS instance with default parameters
    const fsrs_FSRS* const fsrs = fsrs_new(DEFAULT_PARAMETERS, DEFAULT_PARAMETERS_LEN);
    if (!fsrs) {
        fprintf(stderr, "Error: Failed to create FSRS instance\n");
        free_fsrs_items(fsrs_items, total_items);
        free_card_histories(review_histories, num_cards);
        return EXIT_FAILURE;
    }
    
    print_parameters(DEFAULT_PARAMETERS, DEFAULT_PARAMETERS_LEN, "DEFAULT_PARAMETERS");
    
    // Create contiguous array for the train_set
    fsrs_FSRSItem* const contiguous_items = create_contiguous_items(fsrs_items, total_items);
    if (!contiguous_items) {
        fprintf(stderr, "Error: Failed to create contiguous items array\n");
        fsrs_free(fsrs);
        free_fsrs_items(fsrs_items, total_items);
        free_card_histories(review_histories, num_cards);
        return EXIT_FAILURE;
    }
    
    // Create FsrsItems structure for optimization
    fsrs_FsrsItems train_set = {
        .items = contiguous_items, 
        .len = total_items
    };

    // Optimize the FSRS model using the created items
    printf("\nOptimizing parameters...\n");
    float* const optimized_parameters = fsrs_compute_parameters(fsrs, &train_set);
    
    if (optimized_parameters) {
        print_parameters(optimized_parameters, DEFAULT_PARAMETERS_LEN, "OPTIMIZED_PARAMETERS");
        
        // Clean up optimized parameters
        fsrs_parameters_free(optimized_parameters);
    } else {
        fprintf(stderr, "Error: Parameter optimization failed!\n");
    }
    
    // Clean up
    free(contiguous_items);  // Free the contiguous array
    free_fsrs_items(fsrs_items, total_items);
    free_card_histories(review_histories, num_cards);
    fsrs_free(fsrs);
    
    printf("\nOptimization complete!\n");
    return optimized_parameters ? EXIT_SUCCESS : EXIT_FAILURE;
}

