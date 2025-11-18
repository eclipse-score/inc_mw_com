# inc_mw_com
Incubation repository for interprocess communication framework

# Building examples

Examples can be built from examples directory by passing desired IPC adapter as feature.

For build:
```
inc_mw_com$ cargo build --example basic-consumer-producer 
```

For mock test:
```
inc_mw_com$ cargo test --test basic-consumer-producer-test 
```

For mock Build and Run:
```
inc_mw_com/com-api$ cargo run --example basic-consumer-producer 
```
