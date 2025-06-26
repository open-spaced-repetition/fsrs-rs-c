#ifndef _FSRS_H
#define _FSRS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


typedef struct fsrs_FSRS fsrs_FSRS;

typedef struct fsrs_FSRSReview {
  uint32_t rating;
  uint32_t delta_t;
} fsrs_FSRSReview;

typedef struct fsrs_FSRSItem {
  struct fsrs_FSRSReview *reviews;
  size_t len;
} fsrs_FSRSItem;

typedef struct fsrs_FsrsItems {
  struct fsrs_FSRSItem *items;
  size_t len;
} fsrs_FsrsItems;

typedef struct fsrs_FsrsReviews {
  struct fsrs_FSRSReview *reviews;
  size_t len;
} fsrs_FsrsReviews;

typedef struct fsrs_MemoryState {
  float stability;
  float difficulty;
} fsrs_MemoryState;

typedef struct fsrs_ItemState {
  struct fsrs_MemoryState memory;
  float interval;
} fsrs_ItemState;

typedef struct fsrs_NextStates {
  struct fsrs_ItemState again;
  struct fsrs_ItemState hard;
  struct fsrs_ItemState good;
  struct fsrs_ItemState easy;
} fsrs_NextStates;

/**
 * Computes the parameters for a given train set.
 *
 * # Safety
 *
 * The `fsrs` pointer must be a valid pointer to an FSRS instance.
 * The `train_set` pointer must be a valid pointer to an array of FSRSItem.
 */
float *fsrs_compute_parameters(const struct fsrs_FSRS *fsrs, struct fsrs_FsrsItems *train_set);

/**
 * Frees the memory allocated for an FSRS instance.
 *
 * # Safety
 *
 * The `fsrs` pointer must be a valid pointer to an FSRS instance created by `fsrs_new`.
 */
void fsrs_free(const struct fsrs_FSRS *fsrs);

/**
 * Frees the memory allocated for an FSRSItem instance.
 *
 * # Safety
 *
 * The `item` pointer must be a valid pointer to an FSRSItem instance created by `fsrs_item_new`.
 */
void fsrs_item_free(struct fsrs_FSRSItem *item);

/**
 * Creates a new FSRSItem instance.
 *
 * # Safety
 *
 * The `reviews` pointer must be a valid pointer to an array of FSRSReview.
 */
struct fsrs_FSRSItem *fsrs_item_new(struct fsrs_FsrsReviews *reviews);

/**
 * Frees the memory allocated for a MemoryState instance.
 *
 * # Safety
 *
 * The `memory_state` pointer must be a valid pointer to a MemoryState instance created by `fsrs_memory_state_new`.
 */
void fsrs_memory_state_free(struct fsrs_MemoryState *memory_state);

/**
 * Creates a new MemoryState instance.
 */
struct fsrs_MemoryState *fsrs_memory_state_new(float stability, float difficulty);

/**
 * Creates a new FSRS instance.
 *
 * # Safety
 *
 * The `parameters` pointer must be a valid pointer to an array of f32 with `len` elements.
 */
const struct fsrs_FSRS *fsrs_new(const float *parameters, size_t len);

/**
 * Computes the next states for a card.
 *
 * # Safety
 *
 * The `fsrs` pointer must be a valid pointer to an FSRS instance.
 * The `memory_state` pointer must be a valid pointer to a MemoryState instance.
 */
struct fsrs_NextStates *fsrs_next_states(const struct fsrs_FSRS *fsrs,
                                         struct fsrs_MemoryState *memory_state,
                                         float desired_retention,
                                         uint32_t days_elapsed);

/**
 * Get the `again` state from NextStates.
 *
 * # Safety
 *
 * The `next_states` pointer must be a valid pointer to a NextStates instance.
 */
struct fsrs_ItemState fsrs_next_states_again(const struct fsrs_NextStates *next_states);

/**
 * Get the `easy` state from NextStates.
 *
 * # Safety
 *
 * The `next_states` pointer must be a valid pointer to a NextStates instance.
 */
struct fsrs_ItemState fsrs_next_states_easy(const struct fsrs_NextStates *next_states);

/**
 * Frees the memory allocated for a NextStates instance.
 *
 * # Safety
 *
 * The `next_states` pointer must be a valid pointer to a NextStates instance created by `fsrs_next_states`.
 */
void fsrs_next_states_free(struct fsrs_NextStates *next_states);

/**
 * Get the `good` state from NextStates.
 *
 * # Safety
 *
 * The `next_states` pointer must be a valid pointer to a NextStates instance.
 */
struct fsrs_ItemState fsrs_next_states_good(const struct fsrs_NextStates *next_states);

/**
 * Get the `hard` state from NextStates.
 *
 * # Safety
 *
 * The `next_states` pointer must be a valid pointer to a NextStates instance.
 */
struct fsrs_ItemState fsrs_next_states_hard(const struct fsrs_NextStates *next_states);

/**
 * Frees the memory allocated for the parameters.
 *
 * # Safety
 *
 * The `params` pointer must be a valid pointer to the parameters created by `fsrs_compute_parameters`.
 */
void fsrs_parameters_free(float *params);

/**
 * Frees the memory allocated for an FSRSReview instance.
 *
 * # Safety
 *
 * The `review` pointer must be a valid pointer to an FSRSReview instance created by `fsrs_review_new`.
 */
void fsrs_review_free(struct fsrs_FSRSReview *review);

/**
 * Creates a new FSRSReview instance.
 */
struct fsrs_FSRSReview *fsrs_review_new(uint32_t rating, uint32_t delta_t);

#endif  /* _FSRS_H */
