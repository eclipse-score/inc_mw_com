// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use super::*;

//
// TypeTags for internal types
//

macro_rules! impl_type_tag {
    ($($t:ty, $e:expr), *) => {
        $(
            impl TypeTag for $t {
                const TYPE_TAG: Tag = Tag::new($e);
            }
        )*
    };
    () => {
        
    };
}

impl_type_tag!(
    (), *b"@()_____",
    bool, *b"@bool___",
    u8, *b"@u8_____",
    i8, *b"@i8_____",
    u16, *b"@u16____",
    i16, *b"@i16____",
    u32, *b"@u32____",
    i32, *b"@i32____",
    u64, *b"@u64____",
    i64, *b"@i64____",
    u128, *b"@u128___",
    i128, *b"@i128___",
    usize, *b"@usize__",
    isize, *b"@isize__",
    f16, *b"@f16____",
    bf16, *b"@bf16___",
    f32, *b"@f32____",
    tf32, *b"@tf32___",
    f64, *b"@f64____",
    char, *b"@char___",
    str, *b"@str____"
);

impl TypeTag for String {
    const TYPE_TAG: Tag = Tag::from_str("std::string::String");
}

impl<const N: usize> TypeTag for StrConst<N> {
    const TYPE_TAG: Tag = Tag::new(*b"@Str____");
}

// Tuples
macro_rules! impl_typetag_for_tuple {
    ($($T:ident),*) => {
        impl<$($T),*> TypeTag for ($($T,)*)
        where
            $($T: TypeTag,)*
        {
            const TYPE_TAG: Tag = Tag::new(*b"@()_____")
                $(.append_tag($T::TYPE_TAG))*;
        }
    }
}

impl_typetag_for_tuple!(A);
impl_typetag_for_tuple!(A, B);
impl_typetag_for_tuple!(A, B, C);
impl_typetag_for_tuple!(A, B, C, D);
impl_typetag_for_tuple!(A, B, C, D, E);
impl_typetag_for_tuple!(A, B, C, D, E, F);
impl_typetag_for_tuple!(A, B, C, D, E, F, G);
impl_typetag_for_tuple!(A, B, C, D, E, F, G, H);
impl_typetag_for_tuple!(A, B, C, D, E, F, G, H, I);
impl_typetag_for_tuple!(A, B, C, D, E, F, G, H, I, J);
impl_typetag_for_tuple!(A, B, C, D, E, F, G, H, I, J, K);
impl_typetag_for_tuple!(A, B, C, D, E, F, G, H, I, J, K, L);
impl_typetag_for_tuple!(A, B, C, D, E, F, G, H, I, J, K, L, M);
impl_typetag_for_tuple!(A, B, C, D, E, F, G, H, I, J, K, L, M, N);

// Arrays
impl<T, const N: usize> TypeTag for [T; N]
where
    T: TypeTag,
{
    const TYPE_TAG: Tag = Tag::new(*b"@[T;N]__")
        .append_tag(T::TYPE_TAG)
        .append_tag(Tag::from_raw(N as u64));
}

// Slices
impl<T> TypeTag for [T]
where
    T: TypeTag,
{
    const TYPE_TAG: Tag = Tag::new(*b"@[]_____").append_tag(T::TYPE_TAG);
}
