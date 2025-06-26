#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <tgmath.h>
#include <stdbool.h>
#include "fsrs.h"

// Default FSRS parameters (equivalent to DEFAULT_PARAMETERS in Python)
static const float DEFAULT_PARAMETERS[] = {
    0.40255f, 1.18385f, 3.173f, 15.69105f, 7.1949f, 0.5345f, 1.4604f, 0.0046f, 
    1.54575f, 0.1192f, 1.01925f, 1.9395f, 0.11f, 0.29605f, 2.2698f, 0.2315f, 
    2.9898f, 0.51655f, 0.6621f
};
static const size_t DEFAULT_PARAMETERS_LEN = sizeof(DEFAULT_PARAMETERS) / sizeof(DEFAULT_PARAMETERS[0]);

// Structure to represent a card (equivalent to Python Card class)
typedef struct {
    time_t due;                          // Due date as Unix timestamp
    fsrs_MemoryState* memory_state;      // Memory state (NULL for new cards)
    int32_t scheduled_days;              // Scheduled interval in days
    time_t last_review;                  // Last review date as Unix timestamp
} Card;

// Function to create a new card
static Card* card_new(void) {
    Card* const card = malloc(sizeof(Card));
    if (!card) {
        return NULL;
    }
    
    const time_t now = time(NULL);
    *card = (Card){
        .due = now,
        .memory_state = NULL,
        .scheduled_days = 0,
        .last_review = 0
    };
    
    return card;
}

// Function to free a card
static void card_free(Card* card) {
    if (card) {
        if (card->memory_state) {
            fsrs_memory_state_free(card->memory_state);
        }
        free(card);
    }
}

// Function to format and print a time_t as a readable date
static void print_date(const time_t timestamp) {
    const struct tm* const tm_info = localtime(&timestamp);
    if (!tm_info) {
        printf("Invalid date");
        return;
    }
    
    printf("%04d-%02d-%02d %02d:%02d:%02d", 
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
}

// Function to schedule a new card
static void schedule_new_card(void) {
    printf("=== Scheduling a new card ===\n");
    
    // Create a new card
    Card* const card = card_new();
    if (!card) {
        fprintf(stderr, "Error: Failed to create card\n");
        return;
    }
    
    // Set desired retention
    const float desired_retention = 0.9f;
    
    // Create a new FSRS model
    const fsrs_FSRS* const fsrs = fsrs_new(DEFAULT_PARAMETERS, DEFAULT_PARAMETERS_LEN);
    if (!fsrs) {
        fprintf(stderr, "Error: Failed to create FSRS instance\n");
        card_free(card);
        return;
    }
    
    // Get next states for a new card (memory_state = NULL, elapsed_days = 0)
    fsrs_NextStates* const next_states = fsrs_next_states(fsrs, NULL, desired_retention, 0U);
    if (!next_states) {
        fprintf(stderr, "Error: Failed to get next states\n");
        fsrs_free(fsrs);
        card_free(card);
        return;
    }
    
    // Get the intervals for each rating
    const fsrs_ItemState again = fsrs_next_states_again(next_states);
    const fsrs_ItemState hard = fsrs_next_states_hard(next_states);
    const fsrs_ItemState good = fsrs_next_states_good(next_states);
    const fsrs_ItemState easy = fsrs_next_states_easy(next_states);
    
    // Display the intervals for each rating
    printf("Again interval: %.1f days\n", again.interval);
    printf("Hard interval:  %.1f days\n", hard.interval);
    printf("Good interval:  %.1f days\n", good.interval);
    printf("Easy interval:  %.1f days\n", easy.interval);
    
    // Assume the card was reviewed and the rating was 'good'
    const fsrs_ItemState next_state = good;
    const int32_t interval = (int32_t)fmax(1.0, round(next_state.interval));
    
    // Update the card with the new memory state and interval
    card->memory_state = fsrs_memory_state_new(next_state.memory.stability, next_state.memory.difficulty);
    if (!card->memory_state) {
        fprintf(stderr, "Error: Failed to create memory state\n");
        fsrs_next_states_free(next_states);
        fsrs_free(fsrs);
        card_free(card);
        return;
    }
    
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
static void schedule_existing_card(void) {
    printf("\n=== Scheduling an existing card ===\n");
    
    // Create an existing card with memory state and last review date
    Card* const card = card_new();
    if (!card) {
        fprintf(stderr, "Error: Failed to create card\n");
        return;
    }
    
    const time_t now = time(NULL);
    const int32_t days_ago = 7;
    
    card->due = now;
    card->last_review = now - (days_ago * 24 * 60 * 60); // 7 days ago
    card->memory_state = fsrs_memory_state_new(7.0f, 5.0f);
    card->scheduled_days = days_ago;
    
    if (!card->memory_state) {
        fprintf(stderr, "Error: Failed to create initial memory state\n");
        card_free(card);
        return;
    }
    
    // Set desired retention
    const float desired_retention = 0.9f;
    
    // Create a new FSRS model
    const fsrs_FSRS* const fsrs = fsrs_new(DEFAULT_PARAMETERS, DEFAULT_PARAMETERS_LEN);
    if (!fsrs) {
        fprintf(stderr, "Error: Failed to create FSRS instance\n");
        card_free(card);
        return;
    }
    
    // Calculate the elapsed time since the last review
    const uint32_t elapsed_days = (uint32_t)((now - card->last_review) / (24 * 60 * 60));
    
    // Get next states for an existing card
    fsrs_NextStates* const next_states = fsrs_next_states(fsrs, card->memory_state, desired_retention, elapsed_days);
    if (!next_states) {
        fprintf(stderr, "Error: Failed to get next states\n");
        fsrs_free(fsrs);
        card_free(card);
        return;
    }
    
    // Get the intervals for each rating
    const fsrs_ItemState again = fsrs_next_states_again(next_states);
    const fsrs_ItemState hard = fsrs_next_states_hard(next_states);
    const fsrs_ItemState good = fsrs_next_states_good(next_states);
    const fsrs_ItemState easy = fsrs_next_states_easy(next_states);
    
    // Display the intervals for each rating
    printf("Again interval: %.1f days\n", again.interval);
    printf("Hard interval:  %.1f days\n", hard.interval);
    printf("Good interval:  %.1f days\n", good.interval);
    printf("Easy interval:  %.1f days\n", easy.interval);
    
    // Assume the card was reviewed and the rating was 'again'
    const fsrs_ItemState next_state = again;
    const int32_t interval = (int32_t)fmax(1.0, round(next_state.interval));
    
    // Update the card with the new memory state and interval
    // Free the old memory state first
    fsrs_memory_state_free(card->memory_state);
    card->memory_state = fsrs_memory_state_new(next_state.memory.stability, next_state.memory.difficulty);
    
    if (!card->memory_state) {
        fprintf(stderr, "Error: Failed to create new memory state\n");
        fsrs_next_states_free(next_states);
        fsrs_free(fsrs);
        card_free(card);
        return;
    }
    
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

int32_t main(void) {
    schedule_new_card();
    schedule_existing_card();
    return EXIT_SUCCESS;
} 