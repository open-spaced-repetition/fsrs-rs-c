use std::path::Path;

fn main() {
    let header_file = Path::new("include").join("fsrs.h");
    cbindgen::generate(".").unwrap().write_to_file(header_file);
}
