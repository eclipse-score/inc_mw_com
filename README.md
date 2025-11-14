# inc_mw_com
Incubation repository for interprocess communication framework

# Building examples

Examples can be built from examples directory by passing desired IPC adapter as feature.

For mock build:
```
inc_mw_com$ cargo build --example basic-consumer-producer --features "mock"
```

For LoLa build:
```
inc_mw_com/com-api$ cargo build --example basic-consumer-producer --features "lola"
```

For mock test:
```
inc_mw_com$ cargo test --test basic-consumer-producer-test --features "mock"
```

For mock Build and Run:
```
inc_mw_com/com-api$ cargo run --example basic-consumer-producer --features "mock"
```

For LoLa Build and Run:
```
inc_mw_com/com-api$ cargo run --example basic-consumer-producer --features "lola"
```
