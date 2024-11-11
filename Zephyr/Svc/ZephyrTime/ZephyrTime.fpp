module Zephyr {
    @ A component for getting time
    passive component ZephyrTime {

        @ Port to retrieve time
        sync input port timeGetPort: Fw.Time

    }
}