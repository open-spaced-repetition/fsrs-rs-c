use fsrs::{FSRS, FSRSItem, FSRSReview, ItemState, MemoryState, NextStates, Rating};
use std::ffi::{c_char, CStr};
use std::ptr::null_mut;

#[repr(C)]
pub struct FsrsItems {
    pub items: *mut FSRSItem,
    pub len: usize,
}

#[repr(C)]
pub struct FsrsReviews {
    pub reviews: *mut FSRSReview,
    pub len: usize,
}

/// Creates a new FSRS instance.
///
/// # Safety
///
/// The `parameters` pointer must be a valid pointer to an array of f32 with `len` elements.
#[no_mangle]
pub unsafe extern "C" fn fsrs_new(parameters: *const f32, len: usize) -> *mut FSRS {
    let params = if parameters.is_null() {
        None
    } else {
        Some(std::slice::from_raw_parts(parameters, len))
    };
    Box::into_raw(Box::new(FSRS::new(params).unwrap()))
}

/// Frees the memory allocated for an FSRS instance.
///
/// # Safety
///
/// The `fsrs` pointer must be a valid pointer to an FSRS instance created by `fsrs_new`.
#[no_mangle]
pub unsafe extern "C" fn fsrs_free(fsrs: *mut FSRS) {
    if !fsrs.is_null() {
        drop(Box::from_raw(fsrs));
    }
}

/// Computes the next states for a card.
///
/// # Safety
///
/// The `fsrs` pointer must be a valid pointer to an FSRS instance.
/// The `memory_state` pointer must be a valid pointer to a MemoryState instance.
#[no_mangle]
pub unsafe extern "C" fn fsrs_next_states(
    fsrs: *mut FSRS,
    memory_state: *mut MemoryState,
    desired_retention: f32,
    days_elapsed: u32,
) -> *mut NextStates {
    let fsrs = &*fsrs;
    let memory_state = if memory_state.is_null() {
        None
    } else {
        Some((*memory_state).clone())
    };
    let next_states = fsrs
        .next_states(memory_state, desired_retention, days_elapsed)
        .unwrap();
    Box::into_raw(Box::new(next_states))
}

/// Frees the memory allocated for a NextStates instance.
///
/// # Safety
///
  /// The `next_states` pointer must be a valid pointer to a NextStates instance created by `fsrs_next_states`.
#[no_mangle]
pub unsafe extern "C" fn fsrs_next_states_free(next_states: *mut NextStates) {
    if !next_states.is_null() {
        drop(Box::from_raw(next_states));
    }
}

/// Get the `again` state from NextStates.
///
/// # Safety
///
/// The `next_states` pointer must be a valid pointer to a NextStates instance.
#[no_mangle]
pub unsafe extern "C" fn fsrs_next_states_again(next_states: *const NextStates) -> ItemState {
    (*next_states).again.clone()
}

/// Get the `hard` state from NextStates.
///
/// # Safety
///
/// The `next_states` pointer must be a valid pointer to a NextStates instance.
#[no_mangle]
pub unsafe extern "C" fn fsrs_next_states_hard(next_states: *const NextStates) -> ItemState {
    (*next_states).hard.clone()
}

/// Get the `good` state from NextStates.
///
/// # Safety
///
/// The `next_states` pointer must be a valid pointer to a NextStates instance.
#[no_mangle]
pub unsafe extern "C" fn fsrs_next_states_good(next_states: *const NextStates) -> ItemState {
    (*next_states).good.clone()
}

/// Get the `easy` state from NextStates.
///
/// # Safety
///
/// The `next_states` pointer must be a valid pointer to a NextStates instance.
#[no_mangle]
pub unsafe extern "C" fn fsrs_next_states_easy(next_states: *const NextStates) -> ItemState {
    (*next_states).easy.clone()
}

/// Computes the parameters for a given train set.
///
/// # Safety
///
/// The `fsrs` pointer must be a valid pointer to an FSRS instance.
/// The `train_set` pointer must be a valid pointer to an array of FSRSItem.
#[no_mangle]
pub unsafe extern "C" fn fsrs_compute_parameters(
    fsrs: *mut FSRS,
    train_set: *mut FsrsItems,
) -> *mut f32 {
    let fsrs = &mut *fsrs;
    let train_set = std::slice::from_raw_parts((*train_set).items, (*train_set).len).to_vec();
    let params = fsrs
        .compute_parameters(fsrs::ComputeParametersInput {
            train_set,
            progress: None,
            enable_short_term: true,
            num_relearning_steps: None,
        })
        .unwrap_or_default();
    Box::into_raw(params.into_boxed_slice()) as *mut f32
}

/// Frees the memory allocated for the parameters.
///
/// # Safety
///
/// The `params` pointer must be a valid pointer to the parameters created by `fsrs_compute_parameters`.
#[no_mangle]
pub unsafe extern "C" fn fsrs_parameters_free(params: *mut f32) {
    if !params.is_null() {
        drop(Box::from_raw(params));
    }
}

/// Creates a new MemoryState instance.
#[no_mangle]
pub extern "C" fn fsrs_memory_state_new(stability: f32, difficulty: f32) -> *mut MemoryState {
    Box::into_raw(Box::new(MemoryState {
        stability,
        difficulty,
    }))
}

/// Frees the memory allocated for a MemoryState instance.
///
/// # Safety
///
/// The `memory_state` pointer must be a valid pointer to a MemoryState instance created by `fsrs_memory_state_new`.
#[no_mangle]
pub unsafe extern "C" fn fsrs_memory_state_free(memory_state: *mut MemoryState) {
    if !memory_state.is_null() {
        drop(Box::from_raw(memory_state));
    }
}

/// Creates a new FSRSItem instance.
///
/// # Safety
///
/// The `reviews` pointer must be a valid pointer to an array of FSRSReview.
#[no_mangle]
pub unsafe extern "C" fn fsrs_item_new(reviews: *mut FsrsReviews) -> *mut FSRSItem {
    let reviews = std::slice::from_raw_parts((*reviews).reviews, (*reviews).len).to_vec();
    Box::into_raw(Box::new(FSRSItem { reviews }))
}

/// Frees the memory allocated for an FSRSItem instance.
///
/// # Safety
///
/// The `item` pointer must be a valid pointer to an FSRSItem instance created by `fsrs_item_new`.
#[no_mangle]
pub unsafe extern "C" fn fsrs_item_free(item: *mut FSRSItem) {
    if !item.is_null() {
        drop(Box::from_raw(item));
    }
}

/// Creates a new FSRSReview instance.
#[no_mangle]
pub extern "C" fn fsrs_review_new(rating: u32, delta_t: u32) -> *mut FSRSReview {
    Box::into_raw(Box::new(FSRSReview { rating, delta_t }))
}

/// Frees the memory allocated for an FSRSReview instance.
///
/// # Safety
///
/// The `review` pointer must be a valid pointer to an FSRSReview instance created by `fsrs_review_new`.
#[no_mangle]
pub unsafe extern "C" fn fsrs_review_free(review: *mut FSRSReview) {
    if !review.is_null() {
        drop(Box::from_raw(review));
    }
}
