# LearnSy

Artifact for OOPSLA23: Improving Oracle-Guided Inductive Synthesis by Efficient Question Selection

### Install

**Build from source (Test on Ubuntu 16.04)**

1. Install dependencies. gcc $\geq$ 9.1, CMake $\geq$ 3.13, and python3 (preferably $\leq$ 3.5, see step 5) are required to build the project. The other dependencies can be installed as the following.

   ```bash
   $ apt-get install libjsoncpp-dev libgoogle-glog-dev
   $ pip3 install pyparsing==2.4.7 tqdm
   ```

2. Clone LearnSy from the repository.

   ```bash
   $ git clone https://github.com/jiry17/LearnSy.git
   ```

3. Build the whole project.

   ```bash
   $ cd LearnSy; ./install
   ```

4. Script `install` builds an old version of *Z3* to ensure compatibility with *Eusolver*. However, it may fail when the versions of related libraries do not match. 

   Please check whether *Z3* is successfully built by checking whether `libz3.so` is available under directory `thirdparty/my-euphony/thirdparty/z3/build`. If it fails, you can either build a suitable version in the same place or switch the path of *Z3* used by our project by changing `Z3PATH` in `CMakeLists.txt` and `Z3_LIBRARY_PATH` in `thirdparty/run_eusolver`. Please contact us if some subsequent issues emerge.

5. (Optional) Configure the Python interpreter for *Eusolver*. This step is not required unless you want to reproduce those evaluation results related to *Eusolver*.

   Some experiments in our paper take *Eusolver* as the PBE solver. The implementation of *Eusolver* we used requires a Python version of at most 3.5. The Python interpreter for *Eusolver* is set in `thirdparty/run_eusolver`, which is `python3` by default. Therefore, when the default interpreter `python3` has a higher version, the interpreter in this file needs to be replaced properly (e.g., with `python3.5`) to run *Eusolver*.

6.  (Optional) Set a *gurobi* license for *PolyGen*. This step is not required unless you want to reproduce those evaluation results related to *PolyGen*.

   Some experiments in our paper take *PolyGen* as the PBE solver. The implementation of *PolyGen* we used takes *gurobi* as the underlying ILP solver. Therefore, a license of *gurobi* is required to run *PolyGen*. You can get an academic license via the following steps.

   1. Register or log in at the [webside](https://www.gurobi.com/) of gurobi.
   2. Visit the [Free Academic License page](https://www.gurobi.com/downloads/end-user-license-agreement-academic/).
   3. Click ***I Accept These Conditions***.
   4. Get a command like  `grbgetkey x...x` at the bottom of the webpage.
   5. Replace `grbgetkey` with `thirdparty/gurobi912/linux64/bin/grbgetkey` and execute this command under the root directory of the project.
   6. Test whether the license works normally by executing `thirdparty/gurobi912/linux64/bin/gurobi.sh` under the root directory of the project. 

**Download docker container**

We also release a docker container in which this project is already built at `~/LearnSy`. This container can be downloaded and executed using the following commands.

```bash
$ docker pull takanashirikka/learnsy:1.0
$ docker run --rm -it takanashirikka/learnsy:1.0
```

**Note**: Those evaluation results related to PolyGen cannot be reproduced in this container because gurobi cannot be used in a virtual machine. Besides, it is still recommended to build the project from source, since docker may slow down the execution and thus affect the reproduced results.

**Run tests**

1. Test whether the project is successfully built.

   ```bash
   $ cd runner/python
   $ ./run --benchmark phone --dataset CS --solver maxflash --selector learnsy
   ```

   The second to the last line of the output is expected to include an integer (the number of examples used) and a program that returns the first three characters of the input. One possibility of this line is shown below.

   ```
   1 {f:str.substr(Param0,0,3)}
   ```

2. Test whether *Eusolver* is successfully built and correctly configured.

   ```bash
   $ cd runner/python
   $ ./run --benchmark phone --dataset CS --solver eusolver --selector learnsy
   ```

   Similar to the first test case, the second to the last line of the output is expected to include an integer and a program that returns the first three characters of the input.

3. Test whether *Polygen* is successfully built and whether *gurobi* is correctly configured.

   ```bash
   $ cd runner/python
   $ ./run --benchmark max3 --dataset CI --solver polygen --selector learnsy
   ```

   The second to the last line of the output is expected to include an integer and a program that returns the maximum among three inputs. One possibility of this line is shown below.

   ```
   6 {max3:ite(&&(<=(Param0,Param1),<=(Param2,Param1)),Param1,ite(<=(Param2,Param0),Param0,Param2))}
   ```


### Run OGIS solvers

We provide a script `run` under directory `runner/python` to run OGIS solvers involved in our evaluation on a single task. 

```bash
run [-h] --dataset {IS,IR,CS,CI,CB} --benchmark BENCHMARK [--output OUTPUT]
         --solver {eusolver,maxflash,polygen,intsy}
         [--selector {learnsy,randomsy,samplesy,default,biased,significant}]
         [--sample-time SAMPLE_TIME] [--uniform] [--flatten FLATTEN]
```

Script `run` receives the following parameters.

|     Flag      |                  Effect                  |   Default   |
| :-----------: | :--------------------------------------: | :---------: |
|   `dataset`   | The name of the dataset, where `IS` and `IR` represents the sub-datasets of interactive tasks on the domains of string manipulation and program repair, respectively. |     NA      |
|  `benchmark`  | The name of the synthesis tasks. The tasks in each dataset can be found in directory `tests`. |     NA      |
|   `output`    | The file to store the number of used examples, the synthesized program, and the time cost. |    None     |
|   `solver`    | The name of the used PBE solver, where `intsy` represents the interactive synthesis framework we used in Exp 1. |     NA      |
|  `selector`   |      The name of the used selector       |  `learnsy`  |
| `sample-time` | The time limit for SampleSy to perform sampling. This flag is used only when selector `samplesy` is used. |     120     |
|   `uniform`   | To use the trivial prior distribution for LearnSy instead of the learned one. This flag is used only when selector `learnsy` is used. |  disabled   |
|   `flatten`   | The flattening limit of LearnSy. This flag is used only when selector `learnsy` is used. | 3000 or 100 |

**Note**: the default value of `flatten` is 3000 and 100 for interactive and non-interactive tasks, respectively. 

```bash
# Example 1: Run MaxFlash + LearnSy on task 'phone' in dataset CS
$ ./run --benchmark phone --dataset CS --solver maxflash
# Example 2: Run EuSolver + Biased on task 'PRE_75_1000' in dataset CB
$ ./run --benchmark PRE_75_1000 --dataset CB --solver eusolver --selector biased
# Example 3: Set the flattening limit of LearnSy to 0 and then run MaxFlash + LearnSy on task 'phone' in dataset CS
$ ./run --benchmark phone --dataset CS --solver maxflash --flatten 0
```

The output of `run` may include log messages and intermediate results depending on which PBE solver and selector are used. If the script runs normally, the last line of the output should include a float number (the time cost in seconds), the second to the last line should include an integer (the number of examples used) and a program (the synthesized program).

Note that not all combinations of datasets, solvers, and selectors are available since the scopes of some solvers and selectors are limited. Figure `run_usage.pdf` under directory `runner/python` lists the commands corresponding to each setting considered in our evaluation.

### Run experiments

We provide a script `run_exp` under directory `runner/python` to draw the tables in our paper and reproduce the evaluation results.

```bash
run_exp [-h] [-t {3,5,6}] [--restart] [-r REPEAT_NUM] [-tn THREAD_NUM]
```

Script `run_exp` receives the following parameters.

|   Flag    |                  Effect                  |  Default   |
| :-------: | :--------------------------------------: | :--------: |
|    `t`    |     The index of the table to draw.      | All tables |
| `restart` | To clear the cached results. `run_exp` will caches all results under directory `runner/cache`, where the original results we used in our paper are stored initially. Enabling this flag will clear all cache files. |  disabled  |
|    `r`    | The number of times to repeat each execution. We consider only the average performance on all repetitions to reduce the effect of randomness since many PBE solvers and selectors considered in our evaluation are random. |     3      |
|   `tn`    | The number of threads to run the experiment.  It is better to ensure this number is no larger than the number of CPU cores and the size of RAM / 8GB (as the memory limit of execution is 8GB). |     4      |

**Note 1**: Reproducing all evaluation results is time-consuming, which takes about 40 hours on our machine. Using flag `-r 1` can reduce the time cost, but the influence of randomness may be enlarged.

**Note 2**: `run_exp` caches the results of all finished executions. If `run_exp` is interrupted exceptionally, re-running `run_exp` without flag `--restart` will continue the previous execution. Besides, if *Eusolver* and *PolyGen* are not appropriately configured, their executions will be recorded as failed instead of interrupting `run_exp`.


### Reuse this project

**Outline the source code**

The source code of this project is organized as below. Some directories are marked with stars (e.g., `basic`*). They are copied directly from the [PISTool](https://jiry17.github.io/pistool/) library.

|    Directory     |               Description                |
| :--------------: | :--------------------------------------: |
| `include/istool` | The header files of this project. They are organized in the same way as the source code files. |
|     `runner`     | Scripts and auxiliary files to run the experiments. `runner/python`, `runner/cache`, and `runner/model` stores the scripts, the cache files for the evaluation results, and the learned prior distributions for *LearnSy*, respectively. |
|     `basic`*     | Those basic data structures required to describe a synthesis task, such as values (`value.cpp`), programs (`program.cpp`), and specifications (`specification.cpp`). |
|      `ext`*      | The extensions required by synthesizers besides the syntax and semantics. This project uses only the *Z3* interpretations of operators (`ext/z3`), witness functions (`ext/vsa`), and the composition of basic operators (`ext/composed_semantics`). |
|     `parser`     | The parser of tasks in our dataset, including a parser for *SyGuS*-styled tasks and the definitions of those involved values, types, and operations. |
|    `solver`*     | The implementations of the PBE solvers considered in this paper, including the enumerative solver (`solver/enum`), [*MaxFlash*]( https://jiry17.github.io/paper/OOPSLA20.pdf) (`solver/vsa`, `solver/maxflash`), [*PolyGen*](https://jiry17.github.io/paper/OOPSLA21.pdf) (`solver/stun`, `solver/polygen`), and the interface for invoking an externally installed *Eusolver* (`solver/external`). |
|    `selector`    | The implementations of the selectors considered in this paper. `selector/baseline` includes *RandomSy*, *SigInp*, and *Biased*; `selector/samplesy`* includes *SampleSy*; `selector/learnsy` includes *LearnSy*. |
|    `executor`    | The main files of this project, where `executor/run_interactive.cpp` and `executor/run_non_interactive.cpp` invokes an OGIS solver on an interactive task and a non-interactive task, respectively, and `executor/invoker` includes the interfaces for invoking the PBE solvers. |

The implementation of *LearnSy* is mainly included in `selector/learnsy`, whose header files are available in `include/istool/selector/learnsy`. We annotate `include/istool/selector/selector.h` and these header files to outline the usage of each class and each method.

**Invoke *LearnSy* for a new task**

We provide an example in directory `example` to show how to apply *LearnSy* to a new domain. In this example, we consider a list-related synthesis task and show (1) how to define new types and values, (2) how to construct a synthesis task, (3) how to implement a new PBE solver, and at last (4) how to invoke *LearnSy* to select examples for the synthesis procedure.

**Note**: It is not recommended to learn the usage of *LearnSy* from `executor/run_interactive.cpp` and `executor/run_non_interactive.cpp` at the beginning, though they include invocations of *LearnSy*. This is because, in these two files, the usage of *LearnSy* is mixed with the parse of task files, the configurations of the PBE solvers, and the invocations of baseline selectors.



