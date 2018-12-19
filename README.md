# SmartgridSim
NS-3 simulation program of Smart Grid communication over 4G cellular network

## How to run the simulation
### Prerequisites
It is recommended to use Docker so each dependency can be met regardless of the underlying SO that is being used in the host. To this extent, the following items are required:
- A host with any Linux distribution installed
- Docker (version >= 18.09.0)
- An internet connection to download and build resources

### Download the resources
To download the simulation program, just clone this repository:
```bash
git clone https://github.com/alexfrt/smartgridsim.git
cd smartgridsim
```

### How to build the Docker container
There is a Dockerfile that already defines all dependencies and directories structures to support the simulation program. The docker image can be built with the following command:
```bash
docker build -t smartgridsim .
```
That will trigger the Docker image build process, which should run without any user intervention.

### How to use the Docker image to run simulations
After building the Docker image, it is possible to run simulations as defined in main.cc program. The run.sh script is in charge of handling simulation trials with different parameters, thus it reads a list of environment variables that contains the desired configurations. The possible parameters are:

| Variable          | Description                                                       | Default Value             |
| :---------------: | :---------------------------------------------------------------: | :-----------------------: |
| NUM_TRIALS        | The number of trials that should be executed for each experiment  | 30                        |
| SIM_TIME          | The maximum simulation time (aka network elapsed time) in seconds | 5                         |
| WALL_TIME         | The maximum elapsed time in seconds to run the experiment         | 1200                      |
| NUM_METERS        | The number of Smart Meters to simulate                            | 10000 8000 6000 4000 2000 |
| AGG_PERCENTAGES   | The aggregation percentages to be simulated in the experiments    | 0 25 50                   |

If you want to run the simulation with the default values, then just run:
```bash
docker run -ti smartgridsim
```

When the simulation has finished, the docker container will stop running and then the results are available for inspection. To retrieve the outputs, run:
```bash
containerId=$(docker ps --last 1 --filter "ancestor=smartgridsim" --filter="status=exited" -q) # Get the container ID
docker cp $containerId:/workspace/ns-allinone-3.28/ns-3.28/scratch/smartgridsim/outputs . # Copy the outputs from the container
```

That will create an `output` directory under the current working directory, which will contain all results from the simulation.

If it is desirable to change the simulation parameters, it is possible to run the simulation container with the environment variables containing the new configuration values, as follows:

```bash
docker run -e NUM_TRIALS=10 -e WALL_TIME=200 -e NUM_METERS="1000 1500 1700" -e AGG_PERCENTAGES="10 20 30" -ti smartgridsim
```
