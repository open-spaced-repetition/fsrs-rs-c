#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <inttypes.h>
#include "fsrs.h"

#define MAX_CARDS 1000

typedef struct {
    char question[512], answer[512];
    float difficulty, stability;
    int interval, total_reviews, correct_reviews;
    time_t last_review;
} Card;

size_t load_cards(Card cards[], const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return 0;
    
    int count = 0;
    char line[2048];
    while (fgets(line, sizeof(line), file) && count < MAX_CARDS) {
        char* q = strtok(line, "|"), *a = strtok(NULL, "|"), *d = strtok(NULL, "|");
        char* s = strtok(NULL, "|"), *i = strtok(NULL, "|"), *t = strtok(NULL, "|");
        char* tr = strtok(NULL, "|"), *cr = strtok(NULL, "\n");
        
        if (q && a && d && s && i && t && tr && cr) {
            cards[count] = (Card) {
                .question = "",
                .answer = "",
                .difficulty = atof(d),
                .stability = atof(s),
                .interval = atoi(i),
                .last_review = atoll(t),
                .total_reviews = atoi(tr),
                .correct_reviews = atoi(cr)
            };
            strcpy(cards[count].question, q);
            strcpy(cards[count].answer, a);
            count++;
        }
    }
    fclose(file);
    return count;
}

void save_cards(Card cards[], size_t count, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) return;
    
    for (size_t i = 0; i < count; i++) {
        fprintf(file, "%s|%s|%.2f|%.2f|%" PRId32 "|%" PRId64 "|%" PRId32 "|%" PRId32 "\n",
                cards[i].question, cards[i].answer,
                cards[i].difficulty, cards[i].stability,
                cards[i].interval, (int64_t)cards[i].last_review,
                cards[i].total_reviews, cards[i].correct_reviews);
    }
    fclose(file);
}

float* optimize_parameters(Card cards[], size_t card_count, const fsrs_FSRS* fsrs) {
    if (card_count == 0) {
        printf("No cards available for optimization\n");
        return NULL;
    }
    
    printf("Processing %zu cards for optimization...\n", card_count);
    
    // Create array to hold FSRSItem pointers (like optimize.c)
    fsrs_FSRSItem** items = malloc(card_count * sizeof(fsrs_FSRSItem*));
    if (!items) {
        printf("Failed to allocate memory for items array\n");
        return NULL;
    }
    
    size_t valid_items = 0;
    for (size_t i = 0; i < card_count; i++) {
        if (cards[i].total_reviews > 0) {
            int num_reviews = cards[i].total_reviews < 5 ? cards[i].total_reviews : 5;
            fsrs_FSRSReview* reviews = malloc(num_reviews * sizeof(fsrs_FSRSReview));
            if (!reviews) continue;
            
            // Create more realistic synthetic review history based on performance
            float success_rate = (float)cards[i].correct_reviews / cards[i].total_reviews;
            for (int j = 0; j < num_reviews; j++) {
                // More realistic delta_t progression (increasing intervals)
                if (j == 0) {
                    reviews[j].delta_t = 0;  // First review always 0
                } else if (j == 1) {
                    reviews[j].delta_t = 1;  // Day after first review
                } else {
                    reviews[j].delta_t = (j - 1) * 3 + 1;  // Increasing intervals
                }
                
                // More realistic rating distribution based on success rate
                if (success_rate > 0.8f) {
                    reviews[j].rating = (j % 3 == 0) ? 4 : 3;  // Mostly good/easy
                } else if (success_rate > 0.6f) {
                    reviews[j].rating = 3;  // Mostly good
                } else if (success_rate > 0.4f) {
                    reviews[j].rating = (j % 2) ? 3 : 2;  // Mix of good and hard
                } else {
                    reviews[j].rating = (j % 2) ? 2 : 1;  // Mix of hard and again
                }
            }
            
            // Create FsrsReviews wrapper and use fsrs_item_new (like optimize.c)
            fsrs_FsrsReviews review_collection = {reviews, num_reviews};
            fsrs_FSRSItem* item = fsrs_item_new(&review_collection);
            
            if (item) {
                items[valid_items] = item;
                valid_items++;
            }
            
            free(reviews);  // Can free reviews after fsrs_item_new copies the data
        }
    }
    
    printf("Created %zu valid items from review history\n", valid_items);
    
    if (valid_items < 3) {  // Need at least 3 items for optimization
        printf("Need at least 3 items for optimization, only have %zu\n", valid_items);
        for (size_t i = 0; i < valid_items; i++) {
            fsrs_item_free(items[i]);
        }
        free(items);
        return NULL;
    }
    // TODO: optimize parameters

    return NULL;
}

int main() {
    // Set locale for UTF-8 support (required for CJK characters)
    setlocale(LC_ALL, "");

    const float default_params[] = {
        0.40255f, 1.18385f, 3.173f, 15.69105f, 7.1949f, 0.5345f, 
        1.4604f, 0.0046f, 1.54575f, 0.1192f, 1.01925f, 1.9395f, 
        0.11f, 0.29605f, 2.2698f, 0.2315f, 2.9898f, 0.51655f, 0.6621f
    };
    const fsrs_FSRS* fsrs = fsrs_new(default_params, 19);
    
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
            .total_reviews = 5,
            .correct_reviews = 4
        };
        
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
            fsrs_parameters_free(optimized);
        } else {
            printf("Need at least 3 cards with review history for optimization\n");
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
            cards[i].total_reviews++;
            if (rating >= 3) cards[i].correct_reviews++;
            
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
