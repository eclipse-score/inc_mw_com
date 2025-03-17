// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use crate::crypto::{CryptoHasher, Sha1};
use crate::hash::Fnv1a64ConstHasher;
use crate::types::{Coherent, Lockfree, Reloc, TypeTag};

/// Id type
///
/// An Id is a number to identify an indexing type of identification.
/// In differentiation to simple usize indexes Id also knows an invalid value.
/// Throughout the Qor frameworks Id is preferred over flat index types.
///
#[derive(Debug, Clone, Copy, PartialEq, Hash)]
#[repr(transparent)]
pub struct Id(usize);

impl Default for Id {
    fn default() -> Self {
        Id::INVALID
    }
}

impl TypeTag for Id {
    const TYPE_TAG: Tag = Tag::new(*b"@Id_____");
}

impl Coherent for Id {}
unsafe impl Reloc for Id {}
unsafe impl Lockfree for Id {}

impl Id {
    pub const INVALID: Id = Self { 0: usize::MAX };

    /// Create a new id with the given id number
    pub const fn new(id: usize) -> Self {
        Id { 0: id }
    }

    /// Get the Id as index
    pub const fn index(&self) -> usize {
        if self.is_valid() {
            self.0
        } else {
            panic!("invalid index");
        }
    }

    /// Check validity of Id
    pub const fn is_valid(&self) -> bool {
        self.0 != Id::INVALID.0
    }

    /// Invalidate the Id
    pub fn invalidate(&mut self) {
        self.0 = Id::INVALID.0;
    }
}

impl From<u64> for Id {
    fn from(value: u64) -> Self {
        Id::new(value as usize)
    }
}

impl std::fmt::Display for Id {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.is_valid() {
            write!(f, "#{:03}", self.0)
        } else {
            write!(f, "#invalid")
        }
    }
}

/// A name tag
///
/// A tag is a 64 bit identifier with a capacity to be human readable. It is either
///
/// - An eight character string or
/// - A hash value calculated from a longer string
///
#[derive(Clone, Copy, PartialEq, Hash)]
#[repr(transparent)]
pub struct Tag(u64);

impl Default for Tag {
    fn default() -> Self {
        Tag::INVALID
    }
}

impl TypeTag for Tag {
    const TYPE_TAG: Tag = Tag::new(*b"@Tag____");
}

impl Coherent for Tag {}
unsafe impl Reloc for Tag {}
unsafe impl Lockfree for Tag {}

impl Tag {
    pub const INVALID: Tag = Tag { 0: u64::MAX };
    const HASH_BIT: u64 = 1 << 63;
    const HASH_MASK: u64 = Self::HASH_BIT - 1;

    /// Create a new tag from a constant.
    ///
    /// The constant is regarded as text and gets the hash-bit reset.
    #[inline]
    pub const fn new(tag: [u8; 8]) -> Self {
        Tag {
            0: u64::from_le_bytes(tag) & Self::HASH_MASK,
        }
    }

    /// Create a new tag from a raw value
    ///
    /// The value is not altered.
    #[inline]
    pub const fn from_raw(tag: u64) -> Self {
        Tag { 0: tag }
    }

    /// Create a new tag from a constant value
    ///
    /// The value is regarded as "non-text" and gets the hash-bit set.
    #[inline]
    pub const fn from_const(tag: u64) -> Self {
        Tag {
            0: tag | Self::HASH_BIT,
        }
    }

    /// Create a new tag by hashing the passed string.
    #[inline]
    pub const fn from_str(tag: &str) -> Self {
        Self::from_const(Fnv1a64ConstHasher::from_const(tag.as_bytes()).finish())
    }

    /// Create a new tag from existing by adding another tag
    ///
    /// This always causes the tag to be hashed.
    #[inline]
    pub const fn append_tag(self, tag: Tag) -> Self {
        Self::from_const(
            Fnv1a64ConstHasher::from_seed(self.0)
                .write(&tag.value().to_le_bytes())
                .finish(),
        )
    }

    /// Create a new tag from existing by adding a string
    ///
    /// This always causes the tag to be hashed.
    #[inline]
    pub const fn append_str(self, tag: &str) -> Self {
        Self::from_const(
            Fnv1a64ConstHasher::from_seed(self.0)
                .write(tag.as_bytes())
                .finish(),
        )
    }

    /// Check if tag is valid
    #[inline]
    pub const fn is_valid(&self) -> bool {
        self.0 != Self::INVALID.0
    }

    /// Get the tag value
    #[inline]
    pub const fn value(&self) -> u64 {
        self.0
    }

    /// Invalidate the tag
    #[inline]
    pub fn invalidate(&mut self) {
        self.0 = Self::INVALID.0;
    }
}

impl From<u64> for Tag {
    fn from(value: u64) -> Self {
        Tag { 0: value }
    }
}

impl From<&str> for Tag {
    /// Create the Tag from a string value.
    ///
    /// This uses a hashing function to  
    fn from(value: &str) -> Self {
        Self::from_str(value)
    }
}

impl Into<u64> for Tag {
    fn into(self) -> u64 {
        self.0
    }
}

impl Eq for Tag {
    fn assert_receiver_is_total_eq(&self) {}
}

impl std::fmt::Debug for Tag {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.is_valid() {
            let bytes = self.0.to_le_bytes();

            if bytes.iter().all(|&c| (32..=127).contains(&c)) {
                unsafe {
                    write!(
                        f,
                        "#{} {:016x}",
                        std::str::from_utf8_unchecked(&bytes),
                        self.0
                    ) // in debug mode text and hash
                }
            } else {
                write!(f, "#{:016x}", self.0)
            }
        } else {
            write!(f, "#invalid")
        }
    }
}

impl std::fmt::Display for Tag {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.is_valid() {
            let bytes = self.0.to_le_bytes();

            if bytes.iter().all(|&c| (32..=127).contains(&c)) {
                unsafe { write!(f, "#{}", std::str::from_utf8_unchecked(&bytes),) }
            } else {
                write!(f, "#{:016x}", self.0) // only text without hash
            }
        } else {
            write!(f, "#invalid")
        }
    }
}

/// A name label
///
/// A label is an identifier expressed by SHA-1 hash.
///
#[derive(Clone, Copy, PartialEq, Hash)]
#[repr(transparent)]
pub struct Label(<Sha1 as CryptoHasher>::Output);

impl Default for Label {
    fn default() -> Self {
        Label::INVALID
    }
}

impl TypeTag for Label {
    const TYPE_TAG: Tag = Tag::new(*b"@Label__");
}

impl Coherent for Label {}
unsafe impl Reloc for Label {}

impl Label {
    /// Invalid label hash is SHA-1 with all-ones.
    /// As of today, no known byte sequence produces all-ones in SHA-1.
    pub const INVALID: Label = Label::from_raw([u32::MAX, u32::MAX, u32::MAX, u32::MAX, u32::MAX]);

    /// Create a new label from a constant string.
    ///
    #[inline]
    pub fn new(label: &str) -> Self {
        let mut hasher = Sha1::new();
        hasher.write(label.as_bytes());
        Label::from_raw(hasher.finish_and_fetch())
    }

    /// Create a new tag from a raw value
    ///
    /// The value is not altered.
    #[inline]
    pub const fn from_raw(hash: <Sha1 as CryptoHasher>::Output) -> Self {
        Self { 0: hash }
    }

    /// Create a new label from existing by adding a string slice
    pub fn append_str(self, label: &str) -> Self {
        let mut hasher = Sha1::from_seed(self.0);
        hasher.write(label.as_bytes());
        Label::from_raw(hasher.finish_and_fetch())
    }

    /// Check if tag is valid
    #[inline]
    pub fn is_valid(&self) -> bool {
        self.0 != Self::INVALID.0
    }

    /// Get the label value
    #[inline]
    pub const fn value(&self) -> <Sha1 as CryptoHasher>::Output {
        self.0
    }

    /// Invalidate the label
    #[inline]
    pub fn invalidate(&mut self) {
        self.0 = Self::INVALID.0;
    }
}

impl From<<Sha1 as CryptoHasher>::Output> for Label {
    fn from(value: <Sha1 as CryptoHasher>::Output) -> Self {
        Label { 0: value }
    }
}

impl From<&str> for Label {
    /// Create the Label from a string value.
    ///
    /// This uses a hashing function to  
    fn from(value: &str) -> Self {
        Self::new(value)
    }
}

impl Into<<Sha1 as CryptoHasher>::Output> for Label {
    fn into(self) -> <Sha1 as CryptoHasher>::Output {
        self.0
    }
}

impl Eq for Label {
    fn assert_receiver_is_total_eq(&self) {}
}

impl std::fmt::Debug for Label {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.is_valid() {
            write!(f, "#")?;
            for byte in self.0.iter() {
                write!(f, "{:02x}", byte)?;
            }
            write!(f, "")
        } else {
            write!(f, "#invalid")
        }
    }
}

impl std::fmt::Display for Label {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self.is_valid() {
            write!(f, "#")?;
            for byte in self.0.iter() {
                write!(f, "{:02x}", byte)?;
            }
            write!(f, "")
        } else {
            write!(f, "#invalid")
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fmt::Write;

    #[test]
    fn id_tests() {
        // direct value
        let id = Id::new(42);
        assert_eq!(id.index(), 42);

        // Display
        let mut buff = String::new();
        write!(&mut buff, "{}", id).unwrap();
        assert_eq!(buff, "#042");

        // Display
        buff.clear();
        write!(&mut buff, "{}", Id::INVALID).unwrap();
        assert_eq!(buff, "#invalid");
    }

    #[test]
    fn tag_tests() {
        // direct value
        assert_eq!(Tag::new(*b"01234567").value(), 0x3736353433323130u64);

        // hash value
        let tag = Tag::from("/root/folder/file.txt");
        assert_eq!(tag.value(), 0xcbf7932f2aa742bc);

        // direct value into string using Display
        let mut buff = String::new();
        write!(&mut buff, "{:?}", Tag::new(*b"mem_heap")).unwrap();
        assert_eq!(buff, "#mem_heap 706165685f6d656d");

        // hash value into string using Display
        buff.clear();
        write!(&mut buff, "{:?}", Tag::from(0x71C0)).unwrap();
        assert_eq!(buff, "#00000000000071c0");

        // invalid value into string using Display
        buff.clear();
        write!(&mut buff, "{:?}", Tag::INVALID).unwrap();
        assert_eq!(buff, "#invalid");
    }

    #[test]
    fn label_tests() {
        // string
        let label = Label::new("/root/folder/file.txt");
        assert_eq!(
            label.value(),
            [0xeab92194, 0x6563bc3f, 0xee9cdbe3, 0x0fca5ca3, 0x3dde7899]
        );
    }
}
