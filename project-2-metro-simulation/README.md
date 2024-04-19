# Metro Simulation Project

## Overview

This project simulates the operation of a metro system with multiple lines and incorporates multithreading concepts for efficient control and execution. The simulation includes features such as real-time operation, train movement through designated sections, tunnel management, and dynamic train arrivals. For detailed explanations, refer to the project report.

## Features

- **Multithreading:** Utilizes pthread library for concurrent execution of sections, control center, and logging functionalities.

- **Real-time Simulation:** Operates in real-time by synchronizing with the system clock, simulating the specified duration.

- **Multiple Metro Lines:** Supports four metro lines connecting various points in the city.

- **Tunnel Management:** Controls train movement through a single-lane tunnel (C-D), resolving conflicts based on section occupancy.

- **Dynamic Train Arrivals:** Trains arrive at designated points (A, B, E, F) with customizable probabilities, selecting lines randomly.

- **Efficient Tunnel Clearance:** Prioritizes tunnel clearance based on the section with the maximum number of waiting trains.

- **System Overload Handling:** Prevents system overload by notifying trains from other parts of the city to slow down when the total number of trains exceeds a threshold.

- **Train Breakdowns:** Introduces occasional train breakdowns in the tunnel, resulting in additional processing time.

- **Logging:** Records detailed logs for control center events and individual train journeys.

## Usage

1. **Compile the Code:**
    ```bash
    make
    ```

2. **Run the Simulation:**
    ```bash
    ./metro_simulation -s <simulation_time> -p <probability> -d <debug:T or F>
    ```
    - `-s`: Total simulation time in seconds.
    - `-p`: Probability of train arrival at points A, E, or F.
    - `-d`: Debug mode (T for true, F for false).

3. **View Logs:**
    - Control center events are logged in `control-center.log`.
    - Individual train details are logged in `train.log`.

4. **Clean Up:**
    ```bash
    make clean
    ```
    This command removes the executable and log files.

## Requirements

- C++ compiler with support for C++11.
- `pthread` library for multithreading.

## Contributors

- Barış Samed Yakar
- Umur Berkay Karakaş


