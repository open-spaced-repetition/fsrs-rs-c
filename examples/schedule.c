#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "fsrs.h"

// Default FSRS parameters (equivalent to DEFAULT_PARAMETERS in Python)
static const float DEFAULT_PARAMETERS[] = {
    0.40255, 1.18385, 3.173, 15.69105, 7.1949, 0.5345, 1.4604, 0.0046, 
    1.54575, 0.1192, 1.01925, 1.9395, 0.11, 0.29605, 2.2698, 0.2315, 
    2.9898, 0.51655, 0.6621
};
static const size_t DEFAULT_PARAMETERS_LEN = sizeof(DEFAULT_PARAMETERS) / sizeof(DEFAULT_PARAMETERS[0]);

// Structure to represent a card (equivalent to Python Card class)
typedef struct {
    time_t due;                          // Due date as Unix timestamp
    fsrs_MemoryState* memory_state;      // Memory state (NULL for new cards)
    int scheduled_days;                  // Scheduled interval in days
    time_t last_review;                  // Last review date as Unix timestamp
} Card;

// Function to create a new card
Card* card_new(void) {
    Card* card = malloc(sizeof(Card));
    if (!card) return NULL;
    
    time_t now = time(NULL);
    card->due = now;
    card->memory_state = NULL;
    card->scheduled_days = 0;
    card->last_review = 0;
    
    return card;
}

// Function to free a card
void card_free(Card* card) {
    if (card) {
        if (card->memory_state) {
            fsrs_memory_state_free(card->memory_state);
        }
        free(card);
    }
}

// Function to format and print a time_t as a readable date
void print_date(time_t timestamp) {
    struct tm* tm_info = localtime(&timestamp);
    printf("%04d-%02d-%02d %02d:%02d:%02d", 
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
}

// Function to schedule a new card
void schedule_new_card(void) {
    printf("=== Scheduling a new card ===\n");
    
    // Create a new card
    Card* card = card_new();
    if (!card) {
        printf("Failed to create card\n");
        return;
    }
    
    // Set desired retention
    float desired_retention = 0.9f;
    
    // Create a new FSRS model
    const fsrs_FSRS* fsrs = fsrs_new(DEFAULT_PARAMETERS, DEFAULT_PARAMETERS_LEN);
    if (!fsrs) {
        printf("Failed to create FSRS instance\n");
        card_free(card);
        return;
    }
    
    // Get next states for a new card (memory_state = NULL, elapsed_days = 0)
    fsrs_NextStates* next_states = fsrs_next_states(fsrs, NULL, desired_retention, 0);
    if (!next_states) {
        printf("Failed to get next states\n");
        fsrs_free(fsrs);
        card_free(card);
        return;
    }
    
    // Get the intervals for each rating
    fsrs_ItemState again = fsrs_next_states_again(next_states);
    fsrs_ItemState hard = fsrs_next_states_hard(next_states);
    fsrs_ItemState good = fsrs_next_states_good(next_states);
    fsrs_ItemState easy = fsrs_next_states_easy(next_states);
    
    // Display the intervals for each rating
    printf("Again interval: %.1f days\n", again.interval);
    printf("Hard interval: %.1f days\n", hard.interval);
    printf("Good interval: %.1f days\n", good.interval);
    printf("Easy interval: %.1f days\n", easy.interval);
    
    // Assume the card was reviewed and the rating was 'good'
    fsrs_ItemState next_state = good;
    int interval = (int)fmax(1.0f, roundf(next_state.interval));
    
    // Update the card with the new memory state and interval
    card->memory_state = fsrs_memory_state_new(next_state.memory.stability, next_state.memory.difficulty);
    card->scheduled_days = interval;
    card->last_review = time(NULL);
    card->due = card->last_review + (interval * 24 * 60 * 60); // Convert days to seconds
    
    printf("Next review due: ");
    print_date(card->due);
    printf("\n");
    printf("Memory state: stability=%.6f, difficulty=%.6f\n", 
           card->memory_state->stability, card->memory_state->difficulty);
    
    // Cleanup
    fsrs_next_states_free(next_states);
    fsrs_free(fsrs);
    card_free(card);
}

// Function to schedule an existing card
void schedule_existing_card(void) {
    printf("\n=== Scheduling an existing card ===\n");
    
    // Create an existing card with memory state and last review date
    Card* card = card_new();
    if (!card) {
        printf("Failed to create card\n");
        return;
    }
    
    time_t now = time(NULL);
    card->due = now;
    card->last_review = now - (7 * 24 * 60 * 60); // 7 days ago
    card->memory_state = fsrs_memory_state_new(7.0f, 5.0f);
    card->scheduled_days = 7;
    
    // Set desired retention
    float desired_retention = 0.9f;
    
    // Create a new FSRS model
    const fsrs_FSRS* fsrs = fsrs_new(DEFAULT_PARAMETERS, DEFAULT_PARAMETERS_LEN);
    if (!fsrs) {
        printf("Failed to create FSRS instance\n");
        card_free(card);
        return;
    }
    
    // Calculate the elapsed time since the last review
    uint32_t elapsed_days = (uint32_t)((now - card->last_review) / (24 * 60 * 60));
    
    // Get next states for an existing card
    fsrs_NextStates* next_states = fsrs_next_states(fsrs, card->memory_state, desired_retention, elapsed_days);
    if (!next_states) {
        printf("Failed to get next states\n");
        fsrs_free(fsrs);
        card_free(card);
        return;
    }
    
    // Get the intervals for each rating
    fsrs_ItemState again = fsrs_next_states_again(next_states);
    fsrs_ItemState hard = fsrs_next_states_hard(next_states);
    fsrs_ItemState good = fsrs_next_states_good(next_states);
    fsrs_ItemState easy = fsrs_next_states_easy(next_states);
    
    // Display the intervals for each rating
    printf("Again interval: %.1f days\n", again.interval);
    printf("Hard interval: %.1f days\n", hard.interval);
    printf("Good interval: %.1f days\n", good.interval);
    printf("Easy interval: %.1f days\n", easy.interval);
    
    // Assume the card was reviewed and the rating was 'again'
    fsrs_ItemState next_state = again;
    int interval = (int)fmax(1.0f, roundf(next_state.interval));
    
    // Update the card with the new memory state and interval
    // Free the old memory state first
    fsrs_memory_state_free(card->memory_state);
    card->memory_state = fsrs_memory_state_new(next_state.memory.stability, next_state.memory.difficulty);
    card->scheduled_days = interval;
    card->last_review = now;
    card->due = card->last_review + (interval * 24 * 60 * 60); // Convert days to seconds
    
    printf("Next review due: ");
    print_date(card->due);
    printf("\n");
    printf("Memory state: stability=%.6f, difficulty=%.6f\n", 
           card->memory_state->stability, card->memory_state->difficulty);
    
    // Cleanup
    fsrs_next_states_free(next_states);
    fsrs_free(fsrs);
    card_free(card);
}

int main(void) {
    schedule_new_card();
    schedule_existing_card();
    return 0;
} 