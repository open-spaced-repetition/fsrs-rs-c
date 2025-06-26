#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <tgmath.h>
#include "fsrs.h"

int32_t main(void) {
    const float params[] = {
        0.40255f, 1.18385f, 3.173f, 15.69105f, 7.1949f, 0.5345f, 1.4604f, 0.0046f, 
        1.54575f, 0.1192f, 1.01925f, 1.9395f, 0.11f, 0.29605f, 2.2698f, 0.2315f, 
        2.9898f, 0.51655f, 0.6621f
    };
    const size_t params_len = sizeof(params) / sizeof(params[0]);
    
    const fsrs_FSRS* fsrs = fsrs_new(params, params_len);
    if (!fsrs) {
        fprintf(stderr, "Error: Failed to create FSRS instance\n");
        return EXIT_FAILURE;
    }

    const fsrs_FSRSReview reviews[] = {
        {1U, 1U}, {3U, 2U}, {3U, 3U}, {3U, 5U}, {3U, 8U}, {3U, 13U}
    };
    const size_t reviews_len = sizeof(reviews) / sizeof(reviews[0]);
    
    fsrs_FsrsReviews review_history = {
        .reviews = (fsrs_FSRSReview*)reviews,
        .len = reviews_len
    };
    
    fsrs_FSRSItem* item = fsrs_item_new(&review_history);
    if (!item) {
        fprintf(stderr, "Error: Failed to create FSRS item\n");
        fsrs_free(fsrs);
        return EXIT_FAILURE;
    }

    const float desired_retention = 0.9f;
    const uint32_t days_elapsed = 21U;
    
    fsrs_NextStates* next_states = fsrs_next_states(fsrs, NULL, desired_retention, days_elapsed);
    if (!next_states) {
        fprintf(stderr, "Error: Failed to compute next states\n");
        fsrs_item_free(item);
        fsrs_free(fsrs);
        return EXIT_FAILURE;
    }

    const fsrs_ItemState again = fsrs_next_states_again(next_states);
    const fsrs_ItemState hard = fsrs_next_states_hard(next_states);
    const fsrs_ItemState good = fsrs_next_states_good(next_states);
    const fsrs_ItemState easy = fsrs_next_states_easy(next_states);

    printf("Again: stability: %.6f, difficulty: %.6f, interval: %.1f days\n", 
           again.memory.stability, again.memory.difficulty, again.interval);
    printf("Hard:  stability: %.6f, difficulty: %.6f, interval: %.1f days\n", 
           hard.memory.stability, hard.memory.difficulty, hard.interval);
    printf("Good:  stability: %.6f, difficulty: %.6f, interval: %.1f days\n", 
           good.memory.stability, good.memory.difficulty, good.interval);
    printf("Easy:  stability: %.6f, difficulty: %.6f, interval: %.1f days\n", 
           easy.memory.stability, easy.memory.difficulty, easy.interval);

    fsrs_next_states_free(next_states);
    fsrs_item_free(item);
    fsrs_free(fsrs);

    return EXIT_SUCCESS;
}