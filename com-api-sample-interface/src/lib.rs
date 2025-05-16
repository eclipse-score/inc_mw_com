//! This is the "generated" code for an interface that looks like this (pseudo-IDL):
//!
//! ```poor-persons-idl
//! interface Auto {
//!     linkes_rad: Event<Rad>,
//!     auspuff: Event<Auspuff>,
//!     set_blinker_zustand: FnMut(blinkmodus: BlinkModus) -> Result<bool>,
//! }
//! ```

use com_api::{Interface, Reloc};

#[derive(Debug)]
pub struct Rad {}
unsafe impl Reloc for Rad {}

pub struct Auspuff {}
unsafe impl Reloc for Auspuff {}

pub struct AutoInterface {}

/// Generic
impl Interface for AutoInterface {}
