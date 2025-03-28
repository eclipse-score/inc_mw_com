/// # API Design principles
///
/// - We stick to the builder pattern down to a single service
/// - We make all elements mockable. This means we provide traits for the building blocks.
///   We strive for enabling trait objects for mockable entities.
/// - We allow for the usage of heap memory during initialization phase (offer, connect, ...)
///   but prevent heap memory usage during the running phase. Any allocations during the run phase
///   must happen on preallocated memory chunks.
/// - We support data provision on potentially uninitialized memory for efficiency reasons.
/// - We want to be type safe by deriving as much type information as possible from the interface
///   description.
/// - We want to design interfaces such that they're hard to misuse.
/// - Data communicated over IPC need to be position independent.
/// - Data may not reference other data outside its provinence
/// - We provide simple defaults for customization points
/// - Sending / receiving / calling is always done explicitly.
/// - COM API does not enforce the necessity for internal threads or executors.
///
/// # Lower layer implementation principles
///
/// - We add safety nets to detect ABI incompatibilities
///
/// # Supported IPC ABI
///
/// - Primitives
/// - Static lists / strings
/// - Dynamic lists / strings
/// - Key-value
/// - Structures
/// - Tuples

use std::path::Path;
use std::ops::{Deref, DerefMut};
use std::mem::MaybeUninit;

#[derive(Debug)]
enum Error {
    Fail
}

type Result<T> = std::result::Result<T, Error>;

pub trait Builder {
    type Output;
    /// Open point: Should this be &mut self so that this can be turned into a trait object?
    fn build(self) -> Self::Output;
}

use variante2::*;

pub trait Runtime {}

pub trait RuntimeBuilder: Builder where Builder::Output: Runtime {
    fn load_config(&mut self, config: &Path) -> &mut Self;
}

pub struct RuntimeBuilderImpl {}

impl Builder for RuntimeBuilderImpl {
    type Output = ();
    fn build(self) -> Self::Output { unimplemented!() }
}

impl RuntimeBuilder for RuntimeBuilderImpl {
    fn load_config(&mut self, config: &Path) -> &mut Self { unimplemented!() }
}

impl RuntimeBuilderImpl {
    pub fn new() -> Self { unimplemented!() }
}

/// Determines whether a type may be trivially moved without invalidating inner integrity
trait Reloc {}

impl Reloc for () {}
impl Reloc for u32 {}

mod variante1 {
    /// Ideas / open questions:
    /// - Do we need an additional type parameter for metadata? (E2E, timestamp, sender info)
    trait Sample<T>: Deref<Target=T>
    where
        T: Reloc
    {}

    trait SampleMaybeUninit<T>: DerefMut<Target=MaybeUninit<T>>
    where
        T: Reloc
    {
        type Init: SampleMut<T>;

        unsafe fn assume_init(self) -> Self::Init;
        fn write(self, val: T) -> Self::Init;
    }

    trait SampleMut<T>: DerefMut<Target=T>
    where
        T: Reloc
    {
        fn send(self);
    }

    mod server {
        trait Event<T> {
            type SampleUninit<'a>: super::SampleMaybeUninit<T> + 'a;
            fn allocate(&self) -> super::Result<Self::SampleUninit>;
            fn send(&self, val: T);
        }
    }
}

mod variante2 {
    use std::marker::PhantomData;
    use std::mem::MaybeUninit;
    use std::ops::{Deref, DerefMut};
    use std::time::Duration;
    use crate::Reloc;

    struct LolaEvent<T> {
        _event: PhantomData<T>,
    }

    struct LolaBinding<'a, T> {
        data: *mut T,
        event: &'a LolaEvent<T>,
    }

    enum SampleBinding<'a, T> {
        Lola(LolaBinding<'a, T>),
        Test(Box<T>)
    }

    pub struct Sample<'a, T> where T: Reloc {
        inner: SampleBinding<'a, T>,
    }

    impl<'a, T> From<T> for Sample<'a, T> where T: Reloc {
        fn from(value: T) -> Self {
            Self { inner: SampleBinding::Test(Box::new(value)) }
        }
    }

    impl<T> Deref for Sample<T> where T: Reloc {
        type Target = T;

        fn deref(&self) -> &Self::Target {
            match &self.inner {
                SampleBinding::Lola(_lola) => unimplemented!(),
                SampleBinding::Test(test) => test.as_ref(),
            }
        }
    }

    pub struct SampleMut<'a, T> where T: Reloc {
        data: T,
    }

    impl<'a, T> SampleMut<'a, T> where T: Reloc {
        pub fn send(self) -> super::Result<()> {
            unimplemented!()
        }
    }

    impl<'a, T> Deref for SampleMut<'a, T> where T: Reloc {
        type Target = T;

        fn deref(&self) -> &Self::Target {
            &self.data
        }
    }

    impl<'a, T> DerefMut for SampleMut<'a, T> where T: Reloc {
        fn deref_mut(&mut self) -> &mut Self::Target {
            &mut self.data
        }
    }

    pub struct SampleMaybeUninit<'a, T> where T: Reloc {
        data: MaybeUninit<T>,
    }

    impl<'a, T> SampleMaybeUninit<'a, T> where T: Reloc {
        pub unsafe fn assume_init(self) -> SampleMut<'a, T> {
            unsafe {
                SampleMut {
                    data: self.data.assume_init()
                }
            }
        }

        pub fn write(self, val: T) -> SampleMut<'a, T> {
            SampleMut {
                data: val
            }
        }

        pub fn init_default(self) -> SampleMut<'a, T> where T: Default {
            self.write(Default::default())
        }
    }

    impl<'a, T> Deref for SampleMaybeUninit<'a, T> where T: Reloc {
        type Target = MaybeUninit<T>;

        fn deref(&self) -> &Self::Target {
            &self.data
        }
    }

    impl<'a, T> DerefMut for SampleMaybeUninit<'a, T> where T: Reloc {
        fn deref_mut(&mut self) -> &mut Self::Target {
            &mut self.data
        }
    }

    pub trait Publisher {
        type Payload: Reloc;
        fn allocate(&self) -> super::Result<SampleMaybeUninit<Self::Payload>>;
        fn send(&self, val: Self::Payload) -> super::Result<()>;
    }

    pub enum WaitResult {
        SamplesAvailable,
        Expired,
    }

    pub trait Subscriber {
        type Payload: Reloc;
        fn next(&self) -> super::Result<Option<Sample<Self::Payload>>>;
        fn wait(&self, timeout: Option<Duration>) -> WaitResult;
    }

    mod test {
        use super::*;

        struct TestPublisher<T> {}

        impl<T> TestPublisher<T> where T: Reloc {
            pub fn new() -> TestPublisher<T> {
                Self {}
            }
        }

        impl<T> Publisher for TestPublisher<T> where T: Reloc {
            type Payload = T;

            fn allocate(&self) -> crate::Result<SampleMaybeUninit<T>> {
                Ok(SampleMaybeUninit { data: MaybeUninit::uninit() })
            }

            fn send(&self, val: T) -> crate::Result<()> {
                Ok(())
            }
        }

        struct TestSubscriber<T> {}

        impl<T> TestSubscriber<T> where T: Reloc {
            pub fn new() -> Self { Self {} }
        }

        impl<T> Subscriber for TestSubscriber<T> where T: Reloc {
            type Payload = T;

            fn next(&self) -> crate::Result<Option<Sample<Self::Payload>>> {
                Ok(Some(Sample { inner: SampleBinding::Test(Box::new(42)) }))
            }

            fn wait(&self, timeout: Option<Duration>) -> WaitResult {
                WaitResult::SamplesAvailable
            }
        }

        #[test]
        fn receive_stuff() {
            let test_subscriber = TestSubscriber::new();
            for _ in 0..10 {
                match test_subscriber.wait(Some(Duration::from_secs(5))) {
                    WaitResult::SamplesAvailable => {
                        let sample = test_subscriber.next();
                        println!("{}", *sample);
                    }
                    WaitResult::Expired => println!("No sample received"),
                }
            }
        }

        #[test]
        fn send_stuff() {
            let test_publisher = TestPublisher::new();
            for _ in 0..5 {
                let sample = test_publisher.allocate();
                match sample {
                    Ok(mut sample) => {
                        let init_sample = unsafe {
                            *sample.as_mut_ptr() = 42u32;
                            sample.assume_init()
                        };
                        assert!(init_sample.send().is_ok());
                    }
                    Err(e) => eprintln!("Oh my! {:?}", e),
                }
            }
        }
    }
}

mod variant_3 {
    trait Adapter {
        type Event<T> : super::Event<T> where T: Reloc;
    }


    struct MyInterface<A: Adapter> {
        my_a: A::Event<u32>,
        my_b: A::Event<u32>,
    }

    impl<A: Adapter> Interface for MyInterface<Adapter> {
    }

    trait Proxy<A: Adapter, I: Interface> {}


    struct MyProxy<A: Adapter, I: Interface> {
        my_a: MyInterface<A>::Event<u32>::Subscriber,
        my_b: MyInterface<A>::Event<u32>::Subscriber,
    }

    impl<A: Adapter> Proxy<A, MyInterface<A>> for MyProxy<A, MyInterface<A>> {
    }

    trait Skeleton<A: Adapter, I: Interface> {}

    struct MySkeleton<A: Adapter, I: Interface> {
        my_a: MyInterface<A>::Event<u32>::Publisher,
        my_b: MyInterface<A>::Event<u32>::Publisher,
    }

    fn business_logic(proxy: MyProxy<DynAdapter>) {

    }

    fn test() {
        let test_adapter = Box::new(TestAdapter) as Box<dyn Adapter>;
    }
}

/// interface Camera {
///     num_cars: u32
/// }


#[cfg(test)]
mod tests {
    use super::*;


    fn is_sync<T: Sync>(_val: T) {}

    #[test]
    fn builder_is_sync() {
        is_sync(RuntimeBuilderImpl::new());
    }

    #[test]
    fn build_production_runtime() {
        let runtime_builder = RuntimeBuilderImpl::new();
        let runtime = runtime_builder.build();
    }

    impl Reloc for u32 {}
    struct TestUninitPtr {}

    impl SampleMaybeUninit<u32> for TestUninitPtr {
        type Init = ();

        unsafe fn assume_init(self) -> Self::Init {

        }

        fn write(self, val: u32) -> Self::Init {
            todo!()
        }
    }

    #[test]
    fn use_uninit_ptr() {

    }
}
