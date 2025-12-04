# inc_mw_com
Incubation repository for interprocess communication framework

# Building examples

Examples can be built from examples directory by passing desired IPC adapter as feature.

For build:
```
inc_mw_com/com-api$ cargo build --example basic-consumer-producer 
```

For test:
```
inc_mw_com/com-api$ cargo test --test basic-consumer-producer-test 
```

For Build and Run:
```
inc_mw_com/com-api$ cargo run --example basic-consumer-producer 
```
