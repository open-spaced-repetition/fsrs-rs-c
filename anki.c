#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <inttypes.h>
#include <stdbool.h>
#include "fsrs.h"

#define MAX_CARDS 100
#define MAX_REVIEWS 100

typedef struct {
    time_t timestamp;
    int grade;
} Review;

typedef struct {
    char question[51], answer[51];
    float difficulty, stability;
    int interval;
    time_t last_review;
    Review reviews[MAX_REVIEWS];
    size_t review_count;
} Card;

size_t load_cards(Card cards[], const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return 0;
    
    int count = 0;
    char line[4096];

    while (count < MAX_CARDS) {
        // Parse question (first line)
        if (!fgets(line, sizeof(line), file)) break;
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;  // Skip empty lines
        
        strncpy(cards[count].question, line, sizeof(cards[count].question) - 1);
        cards[count].question[sizeof(cards[count].question) - 1] = '\0';

        // Parse answer (second line)
        if (!fgets(line, sizeof(line), file)) break;
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;  // Skip empty lines
        
        strncpy(cards[count].answer, line, sizeof(cards[count].answer) - 1);
        cards[count].answer[sizeof(cards[count].answer) - 1] = '\0';
        
        // Parse review history (third line: timestamp grade timestamp grade ...)
        if (!fgets(line, sizeof(line), file)) break;
        line[strcspn(line, "\n")] = 0;

        // Initialize card defaults
        cards[count].difficulty = 5.0f;
        cards[count].stability = 2.5f;
        cards[count].interval = 1;
        cards[count].last_review = 0;
        cards[count].review_count = 0;
        
        // Parse review history
        if (strlen(line) > 0) {
            char* token = strtok(line, " ");
            while (token && cards[count].review_count < MAX_REVIEWS) {
                // Parse timestamp
                time_t timestamp = atoll(token);
                token = strtok(NULL, " ");
                if (!token) break;
                
                // Parse grade
                int grade = atoi(token);
                
                // Store review
                cards[count].reviews[cards[count].review_count].timestamp = timestamp;
                cards[count].reviews[cards[count].review_count].grade = grade;
                cards[count].review_count++;
                cards[count].last_review = timestamp;
                
                token = strtok(NULL, " ");
            }
        }
        
        count++;
    }
    
    fclose(file);
    return count;
}

void save_cards(Card cards[], size_t count, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) return;
    
    for (size_t i = 0; i < count; i++) {
        // Write question
        fprintf(file, "%s\n", cards[i].question);
        
        // Write answer
        fprintf(file, "%s\n", cards[i].answer);
        
        // Write review history (timestamp grade pairs)
        for (size_t j = 0; j < cards[i].review_count; j++) {
            fprintf(file, "%" PRId64 " %" PRId32, 
                    (int64_t)cards[i].reviews[j].timestamp,
                    cards[i].reviews[j].grade);
            if (j < cards[i].review_count - 1) {
                fprintf(file, " ");
            }
        }
        fprintf(file, "\n");
        
        // Add empty line between cards for readability
        if (i < count - 1) {
            fprintf(file, "\n");
        }
    }
    fclose(file);
}

void save_parameters(const float* params, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open parameter file for writing");
        return;
    }
    for (size_t i = 0; i < 19; i++) {
        fprintf(file, "%.6f\n", params[i]);
    }
    fclose(file);
}

bool load_parameters(float* params, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return false;
    for (size_t i = 0; i < 19; i++) {
        if (fscanf(file, "%f", &params[i]) != 1) {
            fclose(file);
            return false;
        }
    }
    fclose(file);
    return true;
}

// Helper function to create a contiguous array from array of pointers
fsrs_FSRSItem* create_contiguous_items(fsrs_FSRSItem* const* const item_pointers, 
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

// Function to free FSRSItems array
void free_fsrs_items(fsrs_FSRSItem** const items, const size_t count) {
    if (!items) {
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        fsrs_item_free(items[i]);
    }
    free(items);
}

float* optimize_parameters(Card cards[], size_t card_count, const fsrs_FSRS* fsrs) {
    if (card_count == 0) {
        printf("No cards available for optimization\n");
        return NULL;
    }
    
    printf("Processing %zu cards for optimization...\n", card_count);
    
    // Create array to hold all FSRSItem pointers from all cards
    fsrs_FSRSItem** items = malloc(card_count * MAX_REVIEWS * sizeof(fsrs_FSRSItem*));
    if (!items) {
        printf("Failed to allocate memory for items array\n");
        return NULL;
    }
    
    size_t valid_items = 0;
    
    // Process each card and create multiple FSRSItems from its review history
    for (size_t i = 0; i < card_count; i++) {
        if (cards[i].review_count < 2) {
            continue;  // Need at least 2 reviews to create meaningful items
        }
        
        // Create multiple FSRSItems from this card's history (like optimize.c does)
        for (size_t seq_len = 2; seq_len <= cards[i].review_count; seq_len++) {
            fsrs_FSRSReview* reviews = malloc(seq_len * sizeof(fsrs_FSRSReview));
            if (!reviews) continue;

            // Convert the actual review history to FSRSReview format
            bool has_positive_delta_t = false;

            for (size_t j = 0; j < seq_len; j++) {
                if (j == 0) {
                    reviews[j].delta_t = 0;  // First review always 0
                } else {
                    // Calculate days between reviews
                    time_t prev_time = cards[i].reviews[j-1].timestamp;
                    time_t curr_time = cards[i].reviews[j].timestamp;
                    uint32_t delta_days = (uint32_t)((curr_time - prev_time) / (24 * 60 * 60));
                    reviews[j].delta_t = delta_days;
                    if (delta_days > 0) {
                        has_positive_delta_t = true;
                    }
                }
                
                // Use actual grade from review history
                reviews[j].rating = (uint32_t)cards[i].reviews[j].grade;
            }
            
            // Only create items with valid review sequences
            if (has_positive_delta_t && seq_len > 1) {
                fsrs_FsrsReviews review_collection = {reviews, seq_len};
                fsrs_FSRSItem* item = fsrs_item_new(&review_collection);
                
                if (item) {
                    items[valid_items] = item;
                    valid_items++;
                }
            }
            
            free(reviews);
        }
    }
    
    printf("Created %zu valid items from review history\n", valid_items);
    
    if (valid_items < 3) {  // Need sufficient items for meaningful optimization
        printf("Need at least 3 items for optimization, only have %zu\n", valid_items);
        for (size_t i = 0; i < valid_items; i++) {
            fsrs_item_free(items[i]);
        }
        free(items);
        return NULL;
    }

    // Create contiguous array for the train_set
    fsrs_FSRSItem* const contiguous_items = create_contiguous_items(items, valid_items);
    if (!contiguous_items) {
        printf("Error: Failed to create contiguous items array\n");
        free_fsrs_items(items, valid_items);
        return NULL;
    }
    
    // Create FsrsItems structure for optimization
    fsrs_FsrsItems train_set = {
        .items = contiguous_items, 
        .len = valid_items
    };

    // Optimize the FSRS model using the created items
    printf("\nOptimizing parameters...\n");
    float* const optimized_parameters = fsrs_compute_parameters(fsrs, &train_set);
    
    // Clean up
    free(contiguous_items);
    free_fsrs_items(items, valid_items);

    return optimized_parameters;
}

int main() {
    // Set locale for UTF-8 support (required for CJK characters)
    setlocale(LC_ALL, "");
    float params[] = {
        0.40255f, 1.18385f, 3.173f, 15.69105f, 7.1949f, 0.5345f, 
        1.4604f, 0.0046f, 1.54575f, 0.1192f, 1.01925f, 1.9395f, 
        0.11f, 0.29605f, 2.2698f, 0.2315f, 2.9898f, 0.51655f, 0.6621f
    };

    if (load_parameters(params, "fsrs_params.txt")) {
        printf("Loaded parameters from fsrs_params.txt\n");
    } else {
        printf("No custom parameters found, using defaults\n");
    }

    const fsrs_FSRS* fsrs = fsrs_new(params, 19);
    
    Card cards[MAX_CARDS];
    size_t card_count = load_cards(cards, "cards.txt");
    if (card_count == 0) {
        printf("No cards found, creating some sample cards\n");
        
        cards[0] = (Card) {
            .question = "こんにちは (Hello in Japanese)",
            .answer = "Konnichiwa - A greeting meaning 'hello' or 'good afternoon'",
            .difficulty = 5.0f,
            .stability = 2.5f,
            .interval = 3,
            .last_review = time(NULL) - 86400 * 3,
            .review_count = 2
        };
        
        // Add some sample review history
        time_t base_time = time(NULL) - 86400 * 10;
        cards[0].reviews[0] = (Review){base_time, 3};
        cards[0].reviews[1] = (Review){base_time + 86400 * 3, 4};
        
        card_count = 1;
    }
    
    printf("Flashcard App\n1. Review cards\n2. Optimize parameters\n3. Quit\nChoice: ");
    int choice;
    scanf("%d", &choice);
    getchar();
    
    if (choice == 3) {
        save_cards(cards, card_count, "cards.txt");
        fsrs_free(fsrs);
        printf("Goodbye!\n");
        return EXIT_SUCCESS;
    }
    
    if (choice == 2) {
        printf("Optimizing parameters...\n");
        float* optimized = optimize_parameters(cards, card_count, fsrs);
        if (optimized) {
            fsrs_free(fsrs);
            fsrs = fsrs_new(optimized, 19);
            printf("Parameters optimized based on your review history!\n");
            save_parameters(optimized, "fsrs_params.txt");
            printf("Saved new parameters to fsrs_params.txt\n");
            fsrs_parameters_free(optimized);
        } else {
            printf("Need at least 3 items with review history for optimization\n");
        }
    }

    
    time_t now = time(NULL);
    size_t cards_reviewed = 0;
    for (size_t i = 0; i < card_count; i++) {
        if (now - cards[i].last_review >= cards[i].interval * 86400) {
            printf("\nCard: %s\n", cards[i].question);
            printf("Press Enter to see answer...");
            getchar();
            printf("Answer: %s\n", cards[i].answer);
            printf("Rate (1=Again, 2=Hard, 3=Good, 4=Easy, 0=Quit): ");
            
            int rating;
            scanf("%d", &rating);
            getchar();
            
            if (rating == 0) {
                printf("Quitting review session...\n");
                break;
            }
            
            if (rating < 1 || rating > 4) rating = 3;
            
            fsrs_MemoryState* memory = fsrs_memory_state_new(cards[i].stability, cards[i].difficulty);
            uint32_t days_elapsed = (now - cards[i].last_review) / 86400;
            fsrs_NextStates* states = fsrs_next_states(fsrs, memory, 0.9f, days_elapsed);
            
            fsrs_ItemState new_state;
            switch (rating) {
                case 1: new_state = fsrs_next_states_again(states); break;
                case 2: new_state = fsrs_next_states_hard(states); break;
                case 3: new_state = fsrs_next_states_good(states); break;
                case 4: new_state = fsrs_next_states_easy(states); break;
            }
            
            cards[i].difficulty = new_state.memory.difficulty;
            cards[i].stability = new_state.memory.stability;
            cards[i].interval = (int)new_state.interval;
            cards[i].last_review = now;
            
            // Add review to history
            if (cards[i].review_count < MAX_REVIEWS) {
                cards[i].reviews[cards[i].review_count].timestamp = now;
                cards[i].reviews[cards[i].review_count].grade = rating;
                cards[i].review_count++;
            }
            
            printf("Next review in %" PRId32 " days\n", cards[i].interval);
            cards_reviewed++;
            
            fsrs_next_states_free(states);
            fsrs_memory_state_free(memory);
        }
    }
    
    save_cards(cards, card_count, "cards.txt");
    fsrs_free(fsrs);
    if (cards_reviewed > 0) {
        printf("Session complete! Reviewed %zu cards.\n", cards_reviewed);
    } else {
        printf("No cards due for review today.\n");
    }
    return EXIT_SUCCESS;
}
