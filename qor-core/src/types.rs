// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0
use crate::id::Tag;
use std::{fmt::Debug, fmt::Display, hash::Hash};

mod type_tags;

/// Tuple marker trait
pub trait Tuple {}

impl Tuple for () {}
impl<A> Tuple for (A,) {}
impl<A, B> Tuple for (A, B) {}
impl<A, B, C> Tuple for (A, B, C) {}
impl<A, B, C, D> Tuple for (A, B, C, D) {}
impl<A, B, C, D, E> Tuple for (A, B, C, D, E) {}
impl<A, B, C, D, E, F> Tuple for (A, B, C, D, E, F) {}
impl<A, B, C, D, E, F, G> Tuple for (A, B, C, D, E, F, G) {}
impl<A, B, C, D, E, F, G, H> Tuple for (A, B, C, D, E, F, G, H) {}
impl<A, B, C, D, E, F, G, H, I> Tuple for (A, B, C, D, E, F, G, H, I) {}
impl<A, B, C, D, E, F, G, H, I, J> Tuple for (A, B, C, D, E, F, G, H, I, J) {}
impl<A, B, C, D, E, F, G, H, I, J, K> Tuple for (A, B, C, D, E, F, G, H, I, J, K) {}
impl<A, B, C, D, E, F, G, H, I, J, K, L> Tuple for (A, B, C, D, E, F, G, H, I, J, K, L) {}
impl<A, B, C, D, E, F, G, H, I, J, K, L, M> Tuple for (A, B, C, D, E, F, G, H, I, J, K, L, M) {}
impl<A, B, C, D, E, F, G, H, I, J, K, L, M, N> Tuple for (A, B, C, D, E, F, G, H, I, J, K, L, M, N) {}

/// Indicator trait that a type supports compiler-independent type tags.
///
/// Required for use with data in shared memory locations.
pub trait TypeTag {
    /// TYPE_TAG is a unique tag for each type
    const TYPE_TAG: Tag;
}

/// Compiler-independent type IDs for built-in types
///
/// This type id system replaces that of std::any::TypeId and operates on u64 values
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(transparent)]
pub struct TypeId(Tag);

impl TypeTag for TypeId {
    const TYPE_TAG: Tag = Tag::new(*b"@TypeId_");
}

impl Coherent for TypeId {}
unsafe impl Reloc for TypeId {}
unsafe impl Lockfree for TypeId {}

impl TypeId {
    /// The INVALID TypeId
    pub const INVALID: TypeId = TypeId(Tag::INVALID);

    /// Get the TypeId of a type implementing TypeTag trait.
    pub const fn of<T: TypeTag>() -> TypeId {
        TypeId {
            0: <T as TypeTag>::TYPE_TAG,
        }
    }

    pub const fn is_equal<L: TypeTag, R: TypeTag>() -> bool {
        TypeId::of::<L>().0.value() == TypeId::of::<R>().0.value()
    }

    pub const fn is_bool<T: TypeTag>() -> bool {
        Self::is_equal::<T, bool>()
    }

    pub const fn is_u8<T: TypeTag>() -> bool {
        Self::is_equal::<T, u8>()
    }

    pub const fn is_i8<T: TypeTag>() -> bool {
        Self::is_equal::<T, i8>()
    }

    pub const fn is_u16<T: TypeTag>() -> bool {
        Self::is_equal::<T, u16>()
    }

    pub const fn is_i16<T: TypeTag>() -> bool {
        Self::is_equal::<T, i16>()
    }

    pub const fn is_u32<T: TypeTag>() -> bool {
        Self::is_equal::<T, u32>()
    }

    pub const fn is_i32<T: TypeTag>() -> bool {
        Self::is_equal::<T, i32>()
    }

    pub const fn is_u64<T: TypeTag>() -> bool {
        Self::is_equal::<T, u64>()
    }

    pub const fn is_i64<T: TypeTag>() -> bool {
        Self::is_equal::<T, i64>()
    }

    pub const fn is_u128<T: TypeTag>() -> bool {
        Self::is_equal::<T, u128>()
    }

    pub const fn is_i128<T: TypeTag>() -> bool {
        Self::is_equal::<T, i128>()
    }

    pub const fn is_integral<T: TypeTag>() -> bool {
        Self::is_u8::<T>()
            || Self::is_i8::<T>()
            || Self::is_u16::<T>()
            || Self::is_i16::<T>()
            || Self::is_u32::<T>()
            || Self::is_i32::<T>()
            || Self::is_u64::<T>()
            || Self::is_i64::<T>()
            || Self::is_u128::<T>()
            || Self::is_i128::<T>()
    }

    pub const fn is_f16<T: TypeTag>() -> bool {
        Self::is_equal::<T, f16>()
    }

    pub const fn is_bf16<T: TypeTag>() -> bool {
        Self::is_equal::<T, bf16>()
    }

    pub const fn is_f32<T: TypeTag>() -> bool {
        Self::is_equal::<T, f32>()
    }
    pub const fn is_f64<T: TypeTag>() -> bool {
        Self::is_equal::<T, f64>()
    }

    pub const fn is_floating<T: TypeTag>() -> bool {
        Self::is_f16::<T>() || Self::is_bf16::<T>() || Self::is_f32::<T>() || Self::is_f64::<T>()
    }

    pub const fn is_valid(&self) -> bool {
        self.0.is_valid()
    }

    pub const fn tag(&self) -> Tag {
        self.0
    }
}

impl Hash for TypeId {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.0.hash(state);
    }
}

impl Eq for TypeId {}

impl std::fmt::Display for TypeId {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "${}", self.0)
    }
}

/// Trait that returns the ZERO value of a type
/// Zero is defined as the neutral element of addition
pub trait Zero<T: TypeTag> {
    fn zero() -> T;
}

macro_rules! zero_default_impl {
    ($($t:ty)*) => ($(
        /// Implement the Zero trait for the type $t
        impl Zero<$t> for $t {
            fn zero() -> $t {
                0 as $t
            }
        }
    )*)
}

zero_default_impl!(i8 i16 i32 i64 i128 isize u8 u16 u32 u64 u128 usize);
zero_default_impl!(f32 f64);

/// Trait that returns the One value of a type
/// One is defined as the neutral element of multiplication
pub trait One<T: TypeTag> {
    fn one() -> T;
}

macro_rules! one_default_impl {
    ($($t:ty)*) => ($(
        impl One<$t> for $t {
            fn one() -> $t {
                1 as $t
            }
        }
    )*)
}

one_default_impl!(i8 i16 i32 i64 i128 isize u8 u16 u32 u64 u128 usize);
one_default_impl!(f32 f64);

/// Trait that returns the Inf value of a type
/// Inf is the infinity element of that type
pub trait Inf<T: TypeTag> {
    fn inf() -> T;
}

macro_rules! inf_default_impl {
    ($($t:ty)*) => ($(
        impl Inf<$t> for $t {
            fn inf() -> $t {
                <$t>::INFINITY
            }
        }
    )*)
}

inf_default_impl!(f16 bf16 f32 tf32 f64);


/// The coherent trait marks a type as coherent in memory.
///
/// Coherent means that all data of the type is stored in adjacent memory locations (a range)
/// and no data of the type is stored outside this range.
///
/// All Scalar types are Coherent: bool, u8, i8, u16, i16, .. , u128, i128, f32, f64, char
/// Most Rust types like std::String and &str are not Coherent.
///
/// Container types of the standard library are also not coherent. The qor_data crate provides
/// coherent implementations of collections.
pub trait Coherent {}

macro_rules! coherent_default_impl {
    ($($t:ty)*) => ($(
        impl Coherent for $t {}
    )*)
}

coherent_default_impl!(() bool u8 i8 u16 i16 u32 i32 u64 i64 usize isize f16 bf16 f32 f64 char);

// Arrays of Coherents are Coherent
impl<T: Coherent, const N: usize> Coherent for [T; N] {}

// Tuples of Coherent types are Coherent
macro_rules! impl_coherent_for_tuple {
    ($($T:ident),*) => {
        impl<$($T),*> Coherent for ($($T,)*)
        where
            $($T: Coherent,)*
        {}
    }
}

impl_coherent_for_tuple!(A);
impl_coherent_for_tuple!(A, B);
impl_coherent_for_tuple!(A, B, C);
impl_coherent_for_tuple!(A, B, C, D);
impl_coherent_for_tuple!(A, B, C, D, E);
impl_coherent_for_tuple!(A, B, C, D, E, F);
impl_coherent_for_tuple!(A, B, C, D, E, F, G);
impl_coherent_for_tuple!(A, B, C, D, E, F, G, H);
impl_coherent_for_tuple!(A, B, C, D, E, F, G, H, I);
impl_coherent_for_tuple!(A, B, C, D, E, F, G, H, I, J);
impl_coherent_for_tuple!(A, B, C, D, E, F, G, H, I, J, K);
impl_coherent_for_tuple!(A, B, C, D, E, F, G, H, I, J, K, L);
impl_coherent_for_tuple!(A, B, C, D, E, F, G, H, I, J, K, L, M);
impl_coherent_for_tuple!(A, B, C, D, E, F, G, H, I, J, K, L, M, N);

/// The Reloc marker trait defines a type as relocatable in memory.
///
/// Relocatable types do not contain pointers or references. They are well defined independent of the memory space
/// they are located in. The Relocatable trait is essential for data stored in shared memory locations.
///
/// All Scalar types are Reloc: bool, u8, i8, u16, i16, .. , u128, i128, f32, f64, char
/// Most other Rust types, especially those that allow dynamic length data or are generics are _not_ Reloc: &str, String, Vec, HashMap, Box, Rc, Arc, Mutex, ...
///
/// # Safety
///
/// This trait has no safety implications and is only a marker.
pub unsafe trait Reloc {}

macro_rules! reloc_default_impl {
    ($($t:ty)*) => ($(
        unsafe impl Reloc for $t {}
    )*)
}

reloc_default_impl!(() bool u8 i8 u16 i16 u32 i32 u64 i64 usize isize f16 bf16 f32 f64 char);

// Arrays of Relocs are Reloc
unsafe impl<T: Reloc, const N: usize> Reloc for [T; N] {}

// Tuples of Reloc types are Reloc
macro_rules! impl_reloc_for_tuple {
    ($($T:ident),*) => {
        unsafe impl<$($T),*> Reloc for ($($T,)*)
        where
            $($T: Reloc,)*
        {}
    }
}

impl_reloc_for_tuple!(A);
impl_reloc_for_tuple!(A, B);
impl_reloc_for_tuple!(A, B, C);
impl_reloc_for_tuple!(A, B, C, D);
impl_reloc_for_tuple!(A, B, C, D, E);
impl_reloc_for_tuple!(A, B, C, D, E, F);
impl_reloc_for_tuple!(A, B, C, D, E, F, G);
impl_reloc_for_tuple!(A, B, C, D, E, F, G, H);
impl_reloc_for_tuple!(A, B, C, D, E, F, G, H, I);
impl_reloc_for_tuple!(A, B, C, D, E, F, G, H, I, J);
impl_reloc_for_tuple!(A, B, C, D, E, F, G, H, I, J, K);
impl_reloc_for_tuple!(A, B, C, D, E, F, G, H, I, J, K, L);
impl_reloc_for_tuple!(A, B, C, D, E, F, G, H, I, J, K, L, M);
impl_reloc_for_tuple!(A, B, C, D, E, F, G, H, I, J, K, L, M, N);


/// The Lockfree marker trait defines a type as atomically lockfree accessible.
///
/// Lockfree is an extension of Sync and a stronger statement, because a lockfree type is not only
/// free from inner mutability, it also guarantees to not lock the accessing thread during access.
/// Lockfree does not imply Nonblocking. Lockfree types may use short spinlock to prevent data races.
///
/// Types marked as lockfree can be accessed without acquiring a lock.
///
/// Scalar types up to 64bits can safely be assumed Lockfree on 64bit architectures.
///
/// # Safety
///
/// This trait has no safety implications and is only a marker.
pub unsafe trait Lockfree: Sync {}

macro_rules! lockfree_default_impl {
    ($($t:ty)*) => ($(
        unsafe impl Lockfree for $t {}
    )*)
}

// Arrays of Lockfree elements are not Lockfree
// Tuples of Lockfree types are not Lockfree


lockfree_default_impl!(() bool u8 i8 u16 i16 u32 i32 u64 i64 usize isize f16 bf16 f32 f64 char);

/// The Scalar trait declares a type as scalar
///
/// Scalars must be Coherent
/// Scalars should be Reloc
pub trait Scalar: Coherent + Copy + Debug {}

macro_rules! scalar_default_impl {
    ($($t:ty)*) => ($(
        impl Scalar for $t {}
    )*)
}

scalar_default_impl!(bool u8 i8 u16 i16 u32 i32 u64 i64 usize isize f16 bf16 f32 f64 char);

// Arrays of Scalars are Scalars
unsafe impl<const N: usize> Reloc for StrConst<N> {}

/// The Integral trait declares a type as integral
pub trait Integral: Scalar {}

macro_rules! integral_default_impl {
    ($($t:ty)*) => ($(
        impl Integral for $t {}
    )*)
}

integral_default_impl!(u8 i8 u16 i16 u32 i32 u64 i64 usize isize);

/// The Coherent trait declares a type as float
pub trait Floating: Scalar {}

macro_rules! floating_default_impl {
    ($($t:ty)*) => ($(
        impl Floating for $t {}
    )*)
}

floating_default_impl!(f16 bf16 f32 f64);

/// The half-precision IEEE754 float 16 type
#[allow(non_camel_case_types)]
#[allow(dead_code)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct f16 {
    value: u16,
}

impl Display for f16 {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{}", f16::to_f32(*self))
    }
}

impl From<f32> for f16 {
    fn from(f32: f32) -> Self {
        f16::from_f32(f32)
    }
}

impl f16 {
    pub const INFINITY: f16 = f16::from_raw(0x7C00);
    pub const NAN: f16 = f16::from_raw(0x7E00);

    pub const fn from_raw(value: u16) -> f16 {
        f16 { value }
    }

    pub const fn as_raw(&self) -> u16 {
        self.value
    }

    pub const fn to_f32(f16: f16) -> f32 {
        let sign = (f16.value >> 15) & 0x0001;
        let exponent = (f16.value >> 10) & 0x001F;
        let fraction = f16.value & 0x03FF;

        let mut value: u32 = 0;
        value |= (sign as u32) << 31;
        value |= (exponent as u32 + 112) << 23;
        value |= (fraction as u32) << 13;

        f32::from_bits(value)
    }

    pub const fn from_f32(f32: f32) -> f16 {
        let f32: u32 = f32.to_bits();

        let sign = (f32 >> 31) & 0x0001;
        let exponent = (f32 >> 23) & 0x00FF;
        let fraction = f32 & 0x007FFFFF;

        let mut value: u16 = 0;
        value |= (sign << 15) as u16;
        value |= ((exponent - 112) << 10) as u16;
        value |= (fraction >> 13) as u16;

        unsafe { std::mem::transmute(value) }
    }
}

impl Zero<f16> for f16 {
    fn zero() -> f16 {
        f16::from_f32(0.0)
    }
}

/// The brain float 16 type
#[allow(non_camel_case_types)]
#[allow(dead_code)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct bf16 {
    value: u16,
}

impl Display for bf16 {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{}", bf16::to_f32(*self))
    }
}

impl From<f32> for bf16 {
    fn from(f32: f32) -> Self {
        bf16::from_f32(f32)
    }
}

impl bf16 {
    pub const INFINITY: bf16 = bf16::from_raw(0x7F80);
    pub const NAN: bf16 = bf16::from_raw(0x7FC0);

    pub const fn from_raw(value: u16) -> bf16 {
        bf16 { value }
    }

    pub const fn as_raw(&self) -> u16 {
        self.value
    }

    pub const fn to_f32(bf16: bf16) -> f32 {
        let sign = (bf16.value >> 15) & 0x0001;
        let exponent = (bf16.value >> 7) & 0x00FF;
        let fraction = bf16.value & 0x007F;

        let mut value: u32 = 0;
        value |= (sign as u32) << 31;
        value |= (exponent as u32 + 112) << 23;
        value |= (fraction as u32) << 16;

        f32::from_bits(value)
    }

    pub const fn from_f32(f32: f32) -> bf16 {
        let f32: u32 = f32.to_bits();

        let sign = (f32 >> 31) & 0x0001;
        let exponent = (f32 >> 23) & 0x00FF;
        let fraction = f32 & 0x007FFFFF;

        let mut value: u16 = 0;
        value |= (sign << 15) as u16;
        value |= ((exponent - 112) << 7) as u16;
        value |= (fraction >> 16) as u16;

        unsafe { std::mem::transmute(value) }
    }
}

impl Zero<bf16> for bf16 {
    fn zero() -> bf16 {
        bf16::from_f32(0.0)
    }
}


/// The tensor float tf32 type
#[allow(non_camel_case_types)]
#[allow(dead_code)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct tf32 {
    value: u32,
}

impl Display for tf32 {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(f, "{}", tf32::to_f32(*self))
    }
}

impl From<f32> for tf32 {
    fn from(f32: f32) -> Self {
        tf32::from_f32(f32)
    }
}

impl tf32 {
    pub const INFINITY: tf32 = tf32::from_raw(0x7F800000);
    pub const NAN: tf32 = tf32::from_raw(0x7FC00000);

    pub const fn from_raw(value: u32) -> tf32 {
        tf32 { value }
    }

    pub const fn as_raw(&self) -> u32 {
        self.value
    }

    pub const fn to_f32(tf32: tf32) -> f32 {
        f32::from_bits(tf32.value)
    }

    pub const fn from_f32(f32: f32) -> tf32 {
        tf32 { value: f32.to_bits() }
    }
}

impl Zero<tf32> for tf32 {
    fn zero() -> tf32 {
        tf32::from_f32(0.0)
    }
}

/// Constant string type
#[derive(Debug, Copy, Clone, PartialEq)]
pub struct StrConst<const N: usize> {
    length: usize,
    chars: [u8; N],
}

impl<const N: usize> StrConst<N> {
    pub const fn new(chars: [u8; N]) -> Self {
        StrConst { length: N, chars }
    }

    pub fn as_str(&self) -> &str {
        std::str::from_utf8(&self.chars).unwrap()
    }
}

impl<const N: usize> Coherent for StrConst<N> {}
