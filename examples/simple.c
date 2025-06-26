#include <stdio.h>
#include "fsrs.h"

int main() {
    float params[] = {0.40255, 1.18385, 3.173, 15.69105, 7.1949, 0.5345, 1.4604, 0.0046, 1.54575, 0.1192, 1.01925, 1.9395, 0.11, 0.29605, 2.2698, 0.2315, 2.9898, 0.51655, 0.6621};
    const fsrs_FSRS* fsrs = fsrs_new(params, sizeof(params) / sizeof(params[0]));

    fsrs_FSRSReview reviews[] = {{1, 1}, {3, 2}, {3, 3}, {3, 5}, {3, 8}, {3, 13}};
    fsrs_FsrsReviews review_history = {reviews, sizeof(reviews) / sizeof(reviews[0])};
    fsrs_FSRSItem* item = fsrs_item_new(&review_history);

    fsrs_NextStates* next_states = fsrs_next_states(fsrs, NULL, 0.9, 21);

    fsrs_ItemState again = fsrs_next_states_again(next_states);
    fsrs_ItemState hard = fsrs_next_states_hard(next_states);
    fsrs_ItemState good = fsrs_next_states_good(next_states);
    fsrs_ItemState easy = fsrs_next_states_easy(next_states);

    printf("again: stability: %f, difficulty: %f, interval: %f\n", again.memory.stability, again.memory.difficulty, again.interval);
    printf("hard: stability: %f, difficulty: %f, interval: %f\n", hard.memory.stability, hard.memory.difficulty, hard.interval);
    printf("good: stability: %f, difficulty: %f, interval: %f\n", good.memory.stability, good.memory.difficulty, good.interval);
    printf("easy: stability: %f, difficulty: %f, interval: %f\n", easy.memory.stability, easy.memory.difficulty, easy.interval);

    fsrs_next_states_free(next_states);
    fsrs_item_free(item);
    fsrs_free(fsrs);

    return 0;
}