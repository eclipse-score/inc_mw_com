# inc_mw_com
Incubation repository for interprocess communication framework

# Building examples

Examples can be built from examples directory by passing desired IPC adapter as feature.

For mock build:
```
inc_mw_com/com-api$ cargo run --example basic-consumer-producer --features "mock"
```

For LoLa build:
```
inc_mw_com/com-api$ cargo run --example basic-consumer-producer --features "lola"
```
