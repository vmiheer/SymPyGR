//
// Created by milinda on 1/15/18.
/**
*@author Milinda Fernando
*School of Computing, University of Utah
*@brief
*/
//
#include "computeBSSN.h"
#include "test_param.h"
#include "rhs.h"
#include "rhs_cuda.h"
#include "merge_sort.h"

void data_generation_blockwise_mixed(double mean, double std, unsigned int numberOfLevels, unsigned int lower_bound, unsigned int upper_bound, bool isRandom, 
    Block * blkList, double ** var_in_array, double ** var_out_array){

    const unsigned int maxDepth=12;

    // Calculate block sizes
    std::map<int, double> block_sizes;
    for(int i=lower_bound; i<=upper_bound; i++){
        Block blk = Block(0, 0, 0, 2*i, i, maxDepth);
        block_sizes[i] = 1.0*blk.blkSize*BSSN_NUM_VARS*sizeof(double)/1024/1024;
    }

    // Create distribution
    int seed = 100;
    if (isRandom) seed=time(0);
    std::default_random_engine generator(seed);
    std::normal_distribution<double> distribution(mean, std);

    // Generating levels and block structure
    int total_grid_points = 0;
    double total_Megaflops = 0;
    int p[upper_bound-lower_bound+1];
    for (int i=0; i<upper_bound-lower_bound+1; i++) p[i] = 0;

    int level;
    int block_no = 0;
    while (block_no<numberOfLevels){
        level = int(distribution(generator)); // generate a number

        // distribution representation requirement
        if ((level>=lower_bound)&&(level<=upper_bound)) {
            Block & blk=blkList[block_no];
            blk=Block(0, 0, 0, 2*level, level, maxDepth);
            blk.block_no = block_no;

            total_grid_points += blk.blkSize;
            total_Megaflops += MegaflopCount[level];

            block_no++;
            p[level-lower_bound]++;
        }
    }
    bssn::timer::flop_count.setTime((total_Megaflops)/1000);
    bssn::timer::total_points.setTime(total_grid_points*BSSN_NUM_VARS);

    // distribution representation requirement
    std::cout << "---Level distribution---" << std::endl;
    double ram_requirement = 0.0;
    for (int i=0; i<=upper_bound-lower_bound; i++) {
        ram_requirement += block_sizes[i+lower_bound]*p[i]; // this is calculated in MBs

        std::cout << i+lower_bound << ": ";
        std::cout << std::setw(3) << p[i]*100/numberOfLevels << "% " << std::string(p[i]*100/numberOfLevels, '*') << std::endl;
    }
    std::cout << "Total blocks: " << block_no << " | Total grid points: " << 1.0*total_grid_points/1000000 << "x10^6" << " | FlopCount: " << total_Megaflops/1000 << "GigaFlops" << std::endl;
    std::cout << "Total RAM requiremnet for input: " << ram_requirement << "MB" << std::endl;
    std::cout << "Total RAM requiremnet for both input/output: " << ram_requirement*2 << "MB" << std::endl;

    // Checking for available ram
    struct sysinfo myinfo; 
    sysinfo(&myinfo); 
    double total_MB = 1.0*myinfo.totalram/1024/1024; 
    double available_MB = 1.0*myinfo.freeram/1024/1024; 
    std::cout << "Total RAM: " << total_MB << "MB Available RAM: " << available_MB << "MB" << std::endl;

    double remaining = available_MB - ram_requirement*2;
    std::cout << "\nRemaining RAM if the process is going to continue: " << remaining << "MB" << std::endl;
    if (remaining>400){
        std::cout << "Data block generating starts..." << std::endl;
    } else if(remaining>0){
        std::cout << "Are you sure that you want to continue(Y/n):";
        char sure;
        cin >> sure;
        if (tolower(sure)!='y') {
            std::cout << "Program terminated..." << std::endl;
            exit(0);
        }
        std::cout << "Data block generating starts..." << std::endl;
    }else{
        std::cout << "Not enough RAM to process. Program terminated..." << std::endl;
        exit(0);
    }

    // Data block generation
    #pragma omp parallel for num_threads(20)
    for (int index=0; index<numberOfLevels; index++){
        Block & blk=blkList[index];
        const unsigned long unzip_dof=blk.blkSize;

        // Allocating pinned memory in RAM
        double * var_in_per_block;
        CHECK_ERROR(cudaMallocHost((void**)&var_in_per_block, unzip_dof*BSSN_NUM_VARS*sizeof(double)), "var_in_per_block");

        double * var_out_per_block = new double[unzip_dof*BSSN_NUM_VARS];
        CHECK_ERROR(cudaMallocHost((void**)&var_out_per_block, unzip_dof*BSSN_NUM_VARS*sizeof(double)), "var_out_per_block");

        var_in_array[index] = var_in_per_block;
        var_out_array[index] = var_out_per_block;

        double coord[3];
        double u[BSSN_NUM_VARS];
        double x,y,z,hx,hy,hz;
        unsigned int size_x,size_y,size_z;

        x=(double)blk.x;
        y=(double)blk.y;
        z=(double)blk.z;

        hx=0.001; 
        hy=0.001; 
        hz=0.001; 

        size_x=blk.node1D_x;
        size_y=blk.node1D_y;
        size_z=blk.node1D_z;

        for(unsigned int k=0; k<blk.node1D_z; k++){
            for(unsigned int j=0; j<blk.node1D_y; j++){
                for(unsigned int i=0; i<blk.node1D_x; i++){
                    coord[0]=x+i*hx;
                    coord[1]=y+j*hy;
                    coord[2]=z+k*hz;

                    initial_data(u,coord);

                    for(unsigned int var=0; var<BSSN_NUM_VARS; var++){
                        var_in_per_block[var*unzip_dof+k*size_y*size_x+j*size_y+i]=u[var];
                    }
                }
            }
        }
    }
}

void data_generation_blockwise_and_bssn_var_wise_mixed(double mean, double std, unsigned int numberOfLevels, unsigned int lower_bound, unsigned int upper_bound, bool isRandom,
    Block * blkList, double ** var_in_array, double ** var_out_array, double ** var_in, double ** var_out){

    const unsigned int maxDepth=12;

    // Calculate block sizes
    std::map<int, double> block_sizes;
    for(int i=lower_bound; i<=upper_bound; i++){
        Block blk = Block(0, 0, 0, 2*i, i, maxDepth);
        block_sizes[i] = 1.0*(blk.node1D_x*blk.node1D_y*blk.node1D_z)*BSSN_NUM_VARS*sizeof(double)/1024/1024;
    }

    // Create distribution
    int seed = 100;
    if (isRandom) seed=time(0);
    std::default_random_engine generator(seed);
    std::normal_distribution<double> distribution(mean, std);

    // Generating levels and block structure
    int p[upper_bound-lower_bound+1];
    for (int i=0; i<upper_bound-lower_bound+1; i++) p[i] = 0;
    int level;
    int block_no = 0;

    // RAM ---
    unsigned long unzipSz=0;
    while (block_no<numberOfLevels){
        level = int(distribution(generator)); // generate a number

        // distribution representation requirement
        if ((level>=lower_bound)&&(level<=upper_bound)) {
            Block & blk=blkList[block_no];
            blk=Block(0, 0, 0, 2*level, level, maxDepth);
            blk.block_no = block_no;
            // RAM ---
            blk.offset=unzipSz;
            unzipSz+=blk.blkSize;

            block_no++;
            p[level-lower_bound]++;
        }
    }
    // RAM ---
    const unsigned long unzip_dof_cpu=unzipSz;

    // distribution representation requirement
    std::cout << "---Level distribution---" << std::endl;
    double ram_requirement = 0.0;
    for (int i=0; i<=upper_bound-lower_bound; i++) {
        ram_requirement += block_sizes[i+lower_bound]*p[i]; // this is calculated in MBs

        std::cout << i+lower_bound << ": ";
        std::cout << std::setw(3) << p[i]*100/numberOfLevels << "% " << std::string(p[i]*100/numberOfLevels, '*') << std::endl;
    }
    std::cout << "Total blocks: " << block_no << std::endl;
    std::cout << "Total RAM requiremnet for input: " << ram_requirement << "MB" << std::endl;
    std::cout << "Total RAM requiremnet for both input/output: " << ram_requirement*2 << "MB" << std::endl;
    // RAM ---
    std::cout << "Total RAM requiremnet for both GPU and CPU version: " << ram_requirement*2*2 + block_sizes[upper_bound]*210/BSSN_NUM_VARS << "MB" << std::endl;

    // Checking for available ram
    struct sysinfo myinfo; 
    sysinfo(&myinfo); 
    double total_MB = 1.0*myinfo.totalram/1024/1024; 
    double available_MB = 1.0*myinfo.freeram/1024/1024; 
    std::cout << "Total RAM: " << total_MB << "MB Available RAM: " << available_MB << "MB" << std::endl;

    double remaining = available_MB - ram_requirement*2*2 - block_sizes[upper_bound]*210/BSSN_NUM_VARS;
    std::cout << "\nRemaining RAM if the process is going to continue: " << remaining << "MB" << std::endl;
    if (remaining>400){
        std::cout << "Data block generating starts..." << std::endl;
    } else if(remaining>0){
        std::cout << "Are you sure that you want to continue(Y/n):";
        char sure;
        cin >> sure;
        if (tolower(sure)!='y') {
            std::cout << "Program terminated..." << std::endl;
            exit(0);
        }
        std::cout << "Data block generating starts..." << std::endl;
    }else{
        std::cout << "Not enough RAM to process. Program terminated..." << std::endl;
        exit(0);
    }

    // Data block generation
    #pragma omp parallel for num_threads(20)
    for (int index=0; index<numberOfLevels; index++){
        Block & blk=blkList[index];
        const unsigned long unzip_dof=(blk.node1D_x*blk.node1D_y*blk.node1D_z);

        // Allocating pinned memory in RAM
        double * var_in_per_block;
        CHECK_ERROR(cudaMallocHost((void**)&var_in_per_block, unzip_dof*BSSN_NUM_VARS*sizeof(double)), "var_in_per_block");

        double * var_out_per_block = new double[unzip_dof*BSSN_NUM_VARS];
        CHECK_ERROR(cudaMallocHost((void**)&var_out_per_block, unzip_dof*BSSN_NUM_VARS*sizeof(double)), "var_out_per_block");

        var_in_array[index] = var_in_per_block;
        var_out_array[index] = var_out_per_block;

        double coord[3];
        double u[BSSN_NUM_VARS];
        double x,y,z,hx,hy,hz;
        unsigned int size_x,size_y,size_z;
        
        x=(double)blk.x;
        y=(double)blk.y;
        z=(double)blk.z;

        hx=0.001; 
        hy=0.001; 
        hz=0.001; 

        size_x=blk.node1D_x;
        size_y=blk.node1D_y;
        size_z=blk.node1D_z;

        for(unsigned int k=0; k<blk.node1D_z; k++){
            for(unsigned int j=0; j<blk.node1D_y; j++){
                for(unsigned int i=0; i<blk.node1D_x; i++){
                    coord[0]=x+i*hx;
                    coord[1]=y+j*hy;
                    coord[2]=z+k*hz;

                    initial_data(u,coord);

                    for(unsigned int var=0; var<BSSN_NUM_VARS; var++){
                        var_in_per_block[var*unzip_dof+k*size_y*size_x+j*size_y+i]=u[var];
    }
                }
            }
        }
    }

    // RAM ---
    // Data generation for CPU version
    for(int i=0;i<BSSN_NUM_VARS;i++){
        var_in[i] = new double[unzip_dof_cpu];
        var_out[i] = new double[unzip_dof_cpu];
    }
        
    #pragma omp parallel for num_threads(20)
    for(unsigned int blk=0; blk<numberOfLevels; blk++){
        
        double coord[3];
        double u[BSSN_NUM_VARS];

        Block tmpBlock=blkList[blk];
        double x=(double)tmpBlock.x;
        double y=(double)tmpBlock.y;
        double z=(double)tmpBlock.z;

        double hx=0.001;
        double hy=0.001;
        double hz=0.001;

        unsigned int offset=tmpBlock.offset;
        unsigned int size_x=tmpBlock.node1D_x;
        unsigned int size_y=tmpBlock.node1D_y;
        unsigned int size_z=tmpBlock.node1D_z;

        for(unsigned int k=0;k<tmpBlock.node1D_z;k++){
            for(unsigned int j=0;j<tmpBlock.node1D_y;j++){
                for(unsigned int i=0;i<tmpBlock.node1D_x;i++){
                    coord[0]=x+i*hx;
                    coord[1]=y+j*hy;
                    coord[2]=z+k*hz;

                    initial_data(u,coord);

                    for(unsigned int var=0;var<BSSN_NUM_VARS;var++){
                        var_in[var][offset+k*size_y*size_x+j*size_y+i]=u[var];
                        var_out[var][offset+k*size_y*size_x+j*size_y+i]=0;
                    }
                }
            }
        }
    }
}

void GPU_parallelized(unsigned int numberOfLevels, Block * blkList, unsigned int lower_bound, unsigned int upper_bound, double ** var_in_array, double ** var_out_array, bool is_bandwidth_calc=0){ 

    CHECK_ERROR(cudaSetDevice(0), "cudaSetDevice in computeBSSN"); // Set the GPU that we are going to deal with

    // Creating cudaevents to measure the bandwidth times 
    cudaEvent_t start[2], end[2];
    if (is_bandwidth_calc){
        for (int i=0; i<2; i++){
            cudaEventCreate(&start[i]);
            cudaEventCreate(&end[i]);
        }
    }

    // Creating cuda streams for the process
    cudaStream_t stream;
    cudaStream_t streams[steamCountToLevel[0]]; // usually steamCountToLevel[0] should be the max number of streams verify it before execute.
    for (int index=0; index<steamCountToLevel[0]; index++){
        CHECK_ERROR(cudaStreamCreate(&streams[index]), "cudaStream creation");
    }

    // Check for available GPU memory
    size_t free_bytes, total_bytes;
    CHECK_ERROR(cudaMemGetInfo(&free_bytes, &total_bytes), "Available GPU memory checking failed");
    double GPU_capacity_buffer = 10;
    double GPUCapacity = 1.0*free_bytes/1024/1024 - GPU_capacity_buffer;
    std::cout << "Available GPU with buffer of " << GPU_capacity_buffer << ": " << GPUCapacity << " | Total GPU memory: " << total_bytes/1024/1024 << std::endl << std::endl;

    // Sort the data block list
    bssn::timer::t_sorting.start();
    mergeSort(blkList, 0, numberOfLevels-1); // O(nlog(n))
    bssn::timer::t_sorting.stop();

    // Calculate how much data block can be executed at once in GPU
    Block blk;
    int init_block = 0;
    int current_block = 0;
    int total_points = 0;
    int numberOfStreams = 0;
    double current_usage = 0;
    double fixed_usage = 0;
    double prev_usage = 0;
    double actual_usage = 0;
    double malloc_overhead = 3;

    while (current_block<numberOfLevels){
        current_usage=0;
        fixed_usage=0;
        init_block=current_block;

        while ((current_usage+fixed_usage<(GPUCapacity)) && (current_block<numberOfLevels)){
            prev_usage = current_usage+fixed_usage; // usage of previous iteration
            blk = blkList[current_block];
            total_points = blk.blkSize;
            if (blk.blkLevel<5) {
                numberOfStreams = steamCountToLevel[blk.blkLevel];
            }else{
                numberOfStreams = 2;
            }
            if (fixed_usage<numberOfStreams*(138+72)*total_points*sizeof(double)/1024/1024 + 210*malloc_overhead + (current_block-init_block)*malloc_overhead){
                fixed_usage = numberOfStreams*(138+72)*total_points*sizeof(double)/1024/1024 + 210*malloc_overhead + (current_block-init_block)*malloc_overhead; // 5*210 means allocation overhead
            }
            current_usage += (total_points*BSSN_NUM_VARS*sizeof(double)*2)/1024/1024;
            current_block++;
        }
        actual_usage = current_usage+fixed_usage;
        if (current_usage+fixed_usage>(GPUCapacity)){
            current_block--;
            if (init_block>current_block-1) {
                std::cout << "Required GPU memory = " << actual_usage << " Failed to allocate enough memory. Program terminated..." << std::endl;
                exit(0);
            }
            actual_usage = prev_usage;
        }

        // Display the set of blocks selected to process with their GPU usage
        std::cout << "start: " << init_block << " end: " << current_block-1 << "| usage: " << actual_usage << std::endl;

        // Allocating device memory to hold input and output
        double ** dev_var_in_array = new double*[numberOfLevels];
        double ** dev_var_out_array = new double*[numberOfLevels];

        int largest_intermediate_array = 0;
        for (int index=init_block; index<=current_block-1; index++){
            blk = blkList[index];

            if (blk.blkLevel<5) {
                numberOfStreams = steamCountToLevel[blk.blkLevel];
            }else{
                numberOfStreams = 2;
            }

            if (largest_intermediate_array<blk.blkSize*numberOfStreams){
                largest_intermediate_array = blk.blkSize*numberOfStreams;
            }
            bssn::timer::t_malloc_free.start();
            CHECK_ERROR(cudaMalloc((void**)&dev_var_in_array[index], blk.blkSize*BSSN_NUM_VARS*sizeof(double)), "dev_var_in_array[index]");
            CHECK_ERROR(cudaMalloc((void**)&dev_var_out_array[index], blk.blkSize*BSSN_NUM_VARS*sizeof(double)), "dev_var_out_array[index]");
            bssn::timer::t_malloc_free.stop();
        }
        
        // Allocation intermediate arrays
        bssn::timer::t_malloc_free.start();
        int size = largest_intermediate_array * sizeof(double);
        #include "bssnrhs_cuda_variable_malloc.h"
        #include "bssnrhs_cuda_variable_malloc_adv.h"
        #include "bssnrhs_cuda_malloc.h"
        #include "bssnrhs_cuda_malloc_adv.h"
        bssn::timer::t_malloc_free.stop();

        // Start block processing
        if (!is_bandwidth_calc) bssn::timer::t_memcopy_kernel.start();
        int streamIndex;
        int unzip_dof;
        unsigned int sz[3];
        double ptmin[3], ptmax[3];
        unsigned int bflag;
        double dx, dy, dz;
        for(int index=init_block; index<=current_block-1; index++) {
            blk=blkList[index];

            // Call cudasync in the case of block level change occured
            if (index!=init_block && blkList[index-1].blkLevel!=blk.blkLevel) CHECK_ERROR(cudaDeviceSynchronize(), "device sync in computeBSSN level change");

            // Identify stream to schedule
            if (blk.blkLevel<5) {
                numberOfStreams = steamCountToLevel[blk.blkLevel];
            }else{
                numberOfStreams = 2;
            }
            streamIndex = index % numberOfStreams;
            stream = streams[streamIndex];

            // Configure the block specific values
            unzip_dof=blk.blkSize;

            sz[0]=blk.node1D_x; 
            sz[1]=blk.node1D_y;
            sz[2]=blk.node1D_z;

            bflag=0;

            dx=0.1;
            dy=0.1;
            dz=0.1;

            ptmin[0]=0.0;
            ptmin[1]=0.0;
            ptmin[2]=0.0;

            ptmax[0]=1.0;
            ptmax[1]=1.0;
            ptmax[2]=1.0;
            
            std::cout << "GPU - Count: " << std::setw(3) << index << " - Block no: " << std::setw(3) << blk.block_no << " - Bock level: " << std::setw(1) << blk.blkLevel << " - Block size: " << blk.blkSize << std::endl;

            if (is_bandwidth_calc && index==init_block) cudaEventRecord(start[0], stream);
            CHECK_ERROR(cudaMemcpyAsync(dev_var_in_array[index], var_in_array[blk.block_no], BSSN_NUM_VARS*unzip_dof*sizeof(double), cudaMemcpyHostToDevice, stream), "dev_var_in_array[index] cudaMemcpyHostToDevice");

            if (!is_bandwidth_calc){
                cuda_bssnrhs(dev_var_out_array[index], dev_var_in_array[index], unzip_dof, ptmin, ptmax, sz, bflag, stream,
                #include "list_of_args_per_blk.h"
                );
            }

            CHECK_ERROR(cudaMemcpyAsync(var_out_array[blk.block_no], dev_var_out_array[index], BSSN_NUM_VARS*unzip_dof*sizeof(double), cudaMemcpyDeviceToHost, stream), "dev_var_out_array[index] cudaMemcpyDeviceToHost");
            if (is_bandwidth_calc && index==(current_block-1)) cudaEventRecord(end[0], stream);
        }

        CHECK_ERROR(cudaDeviceSynchronize(), "device sync in computeBSSN");

        // Calculating memcopy times
        float eff_memcopy_time;
        if (is_bandwidth_calc){
            cudaEventElapsedTime(&eff_memcopy_time, start[0], end[0]);
            bssn::timer::t_memcopy.setTime(eff_memcopy_time/1000);
        }
        
        if (!is_bandwidth_calc) bssn::timer::t_memcopy_kernel.stop();

        // Release GPU memory
        bssn::timer::t_malloc_free.start();
        #include "bssnrhs_cuda_mdealloc.h"
        #include "bssnrhs_cuda_mdealloc_adv.h"

        for (int index=init_block; index<=current_block-1; index++){
            CHECK_ERROR(cudaFree(dev_var_in_array[index]), "dev_var_in_array[index] cudaFree");
            CHECK_ERROR(cudaFree(dev_var_out_array[index]), "dev_var_out_array[index] cudaFree");
        }
        bssn::timer::t_malloc_free.stop();
    }
    for (int index=0; index<steamCountToLevel[0]; index++){
        CHECK_ERROR(cudaStreamDestroy(streams[index]), "cudaStream destruction");
    }
    return;
}

void GPU_parallelized_async_hybrid(unsigned int numberOfLevels, Block * blkList, unsigned int lower_bound, unsigned int upper_bound, double ** var_in_array, double ** var_out_array, bool is_bandwidth_calc=0){ 
    CHECK_ERROR(cudaSetDevice(0), "cudaSetDevice in computeBSSN"); // Set the GPU that we are going to deal with

    // Creating cudaevents to measure the times 
    cudaEvent_t start[2], end[2];
    if (is_bandwidth_calc){
        for (int i=0; i<2; i++){
            cudaEventCreate(&start[i]);
            cudaEventCreate(&end[i]);
        }
    }

    // Creating cuda streams for the process
    int num_streams = 2;
    if (num_streams<steamCountToLevel[0]) num_streams=steamCountToLevel[0];
    cudaStream_t stream;
    cudaStream_t streams[num_streams]; // usually steamCountToLevel[0] should be the max number of streams verify it before execute.
    for (int index=0; index<num_streams; index++){
        CHECK_ERROR(cudaStreamCreate(&streams[index]), "cudaStream creation");
    }

    // Check for available GPU memory
    size_t free_bytes, total_bytes;
    CHECK_ERROR(cudaMemGetInfo(&free_bytes, &total_bytes), "Available GPU memory checking failed");
    double GPU_capacity_buffer = 10;
    double GPUCapacity = 1.0*free_bytes/1024/1024 - GPU_capacity_buffer;
    if (!is_bandwidth_calc) std::cout << "Available GPU with buffer of " << GPU_capacity_buffer << ": " << GPUCapacity << " | Total GPU memory: " << total_bytes/1024/1024 << std::endl << std::endl;

    // Sort the data block list
    bssn::timer::t_sorting.start();
    mergeSort(blkList, 0, numberOfLevels-1); // O(nlog(n))
    bssn::timer::t_sorting.stop();

    // Calculate how much data block can be executed at once in GPU
    Block blk;
    int init_block = 0;
    int current_block = 0;
    int total_points = 0;
    int numberOfStreams = 0;
    double current_usage = 0;
    double fixed_usage = 0;
    double prev_usage = 0;
    double actual_usage = 0;
    double malloc_overhead = 3;

    while (current_block<numberOfLevels){
        current_usage=0;
        fixed_usage=0;
        init_block=current_block;

        while ((current_usage+fixed_usage<(GPUCapacity)) && (current_block<numberOfLevels)){
            prev_usage = current_usage+fixed_usage; // usage of previous iteration
            blk = blkList[current_block];
            total_points = blk.blkSize;
            if (blk.blkLevel<hybrid_divider) {
                numberOfStreams = steamCountToLevel[blk.blkLevel];
            }else{
                numberOfStreams = 1;
            }
            if (fixed_usage<numberOfStreams*(138+72)*total_points*sizeof(double)/1024/1024 + 210*malloc_overhead + (current_block-init_block)*malloc_overhead){
                fixed_usage = numberOfStreams*(138+72)*total_points*sizeof(double)/1024/1024 + 210*malloc_overhead + (current_block-init_block)*malloc_overhead; // 5*210 means allocation overhead
            }
            current_usage += (total_points*BSSN_NUM_VARS*sizeof(double)*2)/1024/1024;
            current_block++;
        }
        actual_usage = current_usage+fixed_usage;
        if (current_usage+fixed_usage>(GPUCapacity)){
            current_block--;
            if (init_block>current_block-1) {
                std::cout << "Required GPU memory = " << actual_usage << " Failed to allocate enough memory. Program terminated..." << std::endl;
                exit(0);
            }
            actual_usage = prev_usage;
        }

        // Display the set of blocks selected to process with their GPU usage
        if (!is_bandwidth_calc) std::cout << "start: " << init_block << " end: " << current_block-1 << "| usage: " << actual_usage << std::endl;

        // Allocating device memory to hold input and output
        double ** dev_var_in_array = new double*[numberOfLevels];
        double ** dev_var_out_array = new double*[numberOfLevels];

        int largest_intermediate_array = 0;
        for (int index=init_block; index<=current_block-1; index++){
            blk = blkList[index];

            if (blk.blkLevel<hybrid_divider) {
                numberOfStreams = steamCountToLevel[blk.blkLevel];
            }else{
                numberOfStreams = 1;
            }

            if (largest_intermediate_array<blk.blkSize*numberOfStreams){
                largest_intermediate_array = blk.blkSize*numberOfStreams;
            }
            bssn::timer::t_malloc_free.start();
            CHECK_ERROR(cudaMalloc((void**)&dev_var_in_array[index], blk.blkSize*BSSN_NUM_VARS*sizeof(double)), "dev_var_in_array[index]");
            CHECK_ERROR(cudaMalloc((void**)&dev_var_out_array[index], blk.blkSize*BSSN_NUM_VARS*sizeof(double)), "dev_var_out_array[index]");
            bssn::timer::t_malloc_free.stop();
        }
        
        // Allocation intermediate arrays
        bssn::timer::t_malloc_free.start();
        int size = largest_intermediate_array * sizeof(double);
        #include "bssnrhs_cuda_variable_malloc.h"
        #include "bssnrhs_cuda_variable_malloc_adv.h"
        #include "bssnrhs_cuda_malloc.h"
        #include "bssnrhs_cuda_malloc_adv.h"
        bssn::timer::t_malloc_free.stop();
        
        // Start block processing
        if (!is_bandwidth_calc) bssn::timer::t_memcopy_kernel.start();
        int streamIndex;
        int unzip_dof;
        unsigned int sz[3];
        double ptmin[3], ptmax[3];
        unsigned int bflag;
        double dx, dy, dz;
        bool isAsyncStarted = false;
        for(int index=init_block; index<=current_block-1; index++) {
            blk=blkList[index];

            // Configure the block specific values
            unzip_dof=blk.blkSize;

            sz[0]=blk.node1D_x; 
            sz[1]=blk.node1D_y;
            sz[2]=blk.node1D_z;

            bflag=0;

            dx=0.1;
            dy=0.1;
            dz=0.1;

            ptmin[0]=0.0;
            ptmin[1]=0.0;
            ptmin[2]=0.0;

            ptmax[0]=1.0;
            ptmax[1]=1.0;
            ptmax[2]=1.0;

            // Block parallel processing
            if (blk.blkLevel<hybrid_divider){
                // Call cudasync in the case of block level change occured
                if (index!=init_block && blkList[index-1].blkLevel!=blk.blkLevel) {
                    CHECK_ERROR(cudaDeviceSynchronize(), "device sync in computeBSSN level change");
                }

                // Identify stream to schedule
                numberOfStreams = steamCountToLevel[blk.blkLevel];
                streamIndex = index % numberOfStreams;
                stream = streams[streamIndex];

                // std::cout << "GPU - Count: " << std::setw(3) << index << " - Block no: " << std::setw(3) << blk.block_no << " - Bock level: " << std::setw(1) << blk.blkLevel << " - Block size: " << blk.blkSize << std::endl;

                if (is_bandwidth_calc && index==init_block) cudaEventRecord(start[0], stream);
                CHECK_ERROR(cudaMemcpyAsync(dev_var_in_array[index], var_in_array[blk.block_no], BSSN_NUM_VARS*unzip_dof*sizeof(double), cudaMemcpyHostToDevice, stream), "dev_var_in_array[index] cudaMemcpyHostToDevice");

                if (!is_bandwidth_calc){
                    cuda_bssnrhs(dev_var_out_array[index], dev_var_in_array[index], unzip_dof, ptmin, ptmax, sz, bflag, stream,
                        #include "list_of_args_per_blk.h"
                    );
                }

                CHECK_ERROR(cudaMemcpyAsync(var_out_array[blk.block_no], dev_var_out_array[index], BSSN_NUM_VARS*unzip_dof*sizeof(double), cudaMemcpyDeviceToHost, stream), "dev_var_out_array[index] cudaMemcpyDeviceToHost");
                if (is_bandwidth_calc && index==current_block-1) cudaEventRecord(end[0], stream);
            }else{
                // Starting async process

                // Sync any remaining process
                if (!isAsyncStarted) {
                    streamIndex = 0;
                    CHECK_ERROR(cudaDeviceSynchronize(), "device sync in computeBSSN");
                }

                // std::cout << "GPU - Count: " << std::setw(3) << index << " - Block no: " << std::setw(3) << blk.block_no << " - Bock level: " << std::setw(1) << blk.blkLevel << " - Block size: " << blk.blkSize << std::endl;

                if (is_bandwidth_calc && index==init_block) cudaEventRecord(start[0], streams[(index)%2]);
                CHECK_ERROR(cudaMemcpyAsync(dev_var_in_array[index], var_in_array[blk.block_no], BSSN_NUM_VARS*unzip_dof*sizeof(double), cudaMemcpyHostToDevice, streams[(index)%2]), "dev_var_in_array[index] cudaMemcpyHostToDevice");

                if (isAsyncStarted){
                    cudaStreamSynchronize(streams[(index+1)%2]);
                }

                if (!is_bandwidth_calc){
                    cuda_bssnrhs(dev_var_out_array[index], dev_var_in_array[index], unzip_dof, ptmin, ptmax, sz, bflag, streams[(index)%2],
                        #include "list_of_args_per_blk.h"
                    );
                }

                if (isAsyncStarted){
                    CHECK_ERROR(cudaMemcpyAsync(var_out_array[blkList[index-1].block_no], dev_var_out_array[index-1], BSSN_NUM_VARS*blkList[index-1].blkSize*sizeof(double), cudaMemcpyDeviceToHost, streams[(index+1)%2]), "dev_var_out_array[index] cudaMemcpyDeviceToHost");
                }

                //handle last DtoH memcopy
                if(index==current_block-1){
                    CHECK_ERROR(cudaMemcpyAsync(var_out_array[blk.block_no], dev_var_out_array[index], BSSN_NUM_VARS*unzip_dof*sizeof(double), cudaMemcpyDeviceToHost, streams[(index)%2]), "dev_var_out_array[index] cudaMemcpyDeviceToHost");
                }
                if (is_bandwidth_calc && index==current_block-1) cudaEventRecord(end[0], streams[(index)%2]);

                if (!isAsyncStarted) isAsyncStarted=true;
            }
            
        }

        CHECK_ERROR(cudaDeviceSynchronize(), "device sync in computeBSSN");

        // Calculating memcopy times
        float eff_memcopy_time;
        if (is_bandwidth_calc){
            cudaEventElapsedTime(&eff_memcopy_time, start[0], end[0]);
            bssn::timer::t_memcopy.setTime(eff_memcopy_time/1000);
        }
        
        if (!is_bandwidth_calc) bssn::timer::t_memcopy_kernel.stop();

        // Release GPU memory
        bssn::timer::t_malloc_free.start();
        #include "bssnrhs_cuda_mdealloc.h"
        #include "bssnrhs_cuda_mdealloc_adv.h"

        for (int index=init_block; index<=current_block-1; index++){
            CHECK_ERROR(cudaFree(dev_var_in_array[index]), "dev_var_in_array[index] cudaFree");
            CHECK_ERROR(cudaFree(dev_var_out_array[index]), "dev_var_out_array[index] cudaFree");
        }
        bssn::timer::t_malloc_free.stop();
    }
    for (int index=0; index<num_streams; index++){
        CHECK_ERROR(cudaStreamDestroy(streams[index]), "cudaStream destruction");
    }
    return;
}

void GPU_pure_async(unsigned int numberOfLevels, Block * blkList, unsigned int lower_bound, unsigned int upper_bound, double ** var_in_array, double ** var_out_array, bool is_bandwidth_calc=0){ 
    CHECK_ERROR(cudaSetDevice(0), "cudaSetDevice in computeBSSN"); // Set the GPU that we are going to deal with

    // Creating cudaevents to measure the times 
    cudaEvent_t start[2], end[2];
    if (is_bandwidth_calc){
        for (int i=0; i<2; i++){
            cudaEventCreate(&start[i]);
            cudaEventCreate(&end[i]);
        }
    }

    // Creating cuda streams for the process
    int num_streams = 2;
    cudaStream_t stream;
    cudaStream_t streams[num_streams]; // usually steamCountToLevel[0] should be the max number of streams verify it before execute.
    for (int index=0; index<num_streams; index++){
        CHECK_ERROR(cudaStreamCreate(&streams[index]), "cudaStream creation");
    }

    // Check for available GPU memory
    size_t free_bytes, total_bytes;
    CHECK_ERROR(cudaMemGetInfo(&free_bytes, &total_bytes), "Available GPU memory checking failed");
    double GPU_capacity_buffer = 10;
    double GPUCapacity = 1.0*free_bytes/1024/1024 - GPU_capacity_buffer;
    std::cout << "Available GPU with buffer of " << GPU_capacity_buffer << ": " << GPUCapacity << " | Total GPU memory: " << total_bytes/1024/1024 << std::endl << std::endl;

    // Sort the data block list
    bssn::timer::t_sorting.start();
    mergeSort(blkList, 0, numberOfLevels-1); // O(nlog(n))
    bssn::timer::t_sorting.stop();

    // Calculate how much data block can be executed at once in GPU
    Block blk;
    int init_block = 0;
    int current_block = 0;
    int total_points = 0;
    int numberOfStreams = 0;
    double current_usage = 0;
    double fixed_usage = 0;
    double prev_usage = 0;
    double actual_usage = 0;
    double malloc_overhead = 3;

    while (current_block<numberOfLevels){
        current_usage=0;
        fixed_usage=0;
        init_block=current_block;

        while ((current_usage+fixed_usage<(GPUCapacity)) && (current_block<numberOfLevels)){
            prev_usage = current_usage+fixed_usage; // usage of previous iteration
            blk = blkList[current_block];
            total_points = blk.blkSize;
            numberOfStreams = 1;
            if (fixed_usage<numberOfStreams*(138+72)*total_points*sizeof(double)/1024/1024 + 210*malloc_overhead + (current_block-init_block)*malloc_overhead){
                fixed_usage = numberOfStreams*(138+72)*total_points*sizeof(double)/1024/1024 + 210*malloc_overhead + (current_block-init_block)*malloc_overhead; // 5*210 means allocation overhead
            }
            current_usage += (total_points*BSSN_NUM_VARS*sizeof(double)*2)/1024/1024;
            current_block++;
        }
        actual_usage = current_usage+fixed_usage;
        if (current_usage+fixed_usage>(GPUCapacity)){
            current_block--;
            if (init_block>current_block-1) {
                std::cout << "Required GPU memory = " << actual_usage << " Failed to allocate enough memory. Program terminated..." << std::endl;
                exit(0);
            }
            actual_usage = prev_usage;
        }

        // Display the set of blocks selected to process with their GPU usage
        std::cout << "start: " << init_block << " end: " << current_block-1 << "| usage: " << actual_usage << std::endl;

        // Allocating device memory to hold input and output
        double ** dev_var_in_array = new double*[numberOfLevels];
        double ** dev_var_out_array = new double*[numberOfLevels];

        int largest_intermediate_array = 0;
        for (int index=init_block; index<=current_block-1; index++){
            blk = blkList[index];
            numberOfStreams = 1;

            if (largest_intermediate_array<blk.blkSize*numberOfStreams){
                largest_intermediate_array = blk.blkSize*numberOfStreams;
            }
            bssn::timer::t_malloc_free.start();
            CHECK_ERROR(cudaMalloc((void**)&dev_var_in_array[index], blk.blkSize*BSSN_NUM_VARS*sizeof(double)), "dev_var_in_array[index]");
            CHECK_ERROR(cudaMalloc((void**)&dev_var_out_array[index], blk.blkSize*BSSN_NUM_VARS*sizeof(double)), "dev_var_out_array[index]");
            bssn::timer::t_malloc_free.stop();
        }
        
        // Allocation intermediate arrays
        bssn::timer::t_malloc_free.start();
        int size = largest_intermediate_array * sizeof(double);
        #include "bssnrhs_cuda_variable_malloc.h"
        #include "bssnrhs_cuda_variable_malloc_adv.h"
        #include "bssnrhs_cuda_malloc.h"
        #include "bssnrhs_cuda_malloc_adv.h"
        bssn::timer::t_malloc_free.stop();
        
        // Start block processing
        if (!is_bandwidth_calc) bssn::timer::t_memcopy_kernel.start();
        int streamIndex = 0;
        int unzip_dof;
        unsigned int sz[3];
        double ptmin[3], ptmax[3];
        unsigned int bflag;
        double dx, dy, dz;
        bool isAsyncStarted = false;
        for(int index=init_block; index<=current_block-1; index++) {
            blk=blkList[index];

            // Configure the block specific values
            unzip_dof=blk.blkSize;

            sz[0]=blk.node1D_x; 
            sz[1]=blk.node1D_y;
            sz[2]=blk.node1D_z;

            bflag=0;

            dx=0.1;
            dy=0.1;
            dz=0.1;

            ptmin[0]=0.0;
            ptmin[1]=0.0;
            ptmin[2]=0.0;

            ptmax[0]=1.0;
            ptmax[1]=1.0;
            ptmax[2]=1.0;

            // Starting async process

            // Sync any remaining process
            std::cout << "GPU - Count: " << std::setw(3) << index << " - Block no: " << std::setw(3) << blk.block_no << " - Bock level: " << std::setw(1) << blk.blkLevel << " - Block size: " << blk.blkSize << std::endl;

            if (is_bandwidth_calc && index==init_block) cudaEventRecord(start[0], streams[(index)%2]);
            CHECK_ERROR(cudaMemcpyAsync(dev_var_in_array[index], var_in_array[blk.block_no], BSSN_NUM_VARS*unzip_dof*sizeof(double), cudaMemcpyHostToDevice, streams[(index)%2]), "dev_var_in_array[index] cudaMemcpyHostToDevice");

            if (isAsyncStarted){
                cudaStreamSynchronize(streams[(index+1)%2]);
            }

            if (!is_bandwidth_calc){
                cuda_bssnrhs(dev_var_out_array[index], dev_var_in_array[index], unzip_dof, ptmin, ptmax, sz, bflag, streams[(index)%2],
                    #include "list_of_args_per_blk.h"
                );
            }
     
            if (isAsyncStarted){
                CHECK_ERROR(cudaMemcpyAsync(var_out_array[blkList[index-1].block_no], dev_var_out_array[index-1], BSSN_NUM_VARS*blkList[index-1].blkSize*sizeof(double), cudaMemcpyDeviceToHost, streams[(index+1)%2]), "dev_var_out_array[index] cudaMemcpyDeviceToHost");
            }

            //handle last DtoH memcopy
            if(index==current_block-1){
                CHECK_ERROR(cudaMemcpyAsync(var_out_array[blk.block_no], dev_var_out_array[index], BSSN_NUM_VARS*unzip_dof*sizeof(double), cudaMemcpyDeviceToHost, streams[(index)%2]), "dev_var_out_array[index] cudaMemcpyDeviceToHost");
            }
            if (is_bandwidth_calc && index==current_block-1) cudaEventRecord(end[0], streams[(index)%2]);

            if (!isAsyncStarted) isAsyncStarted=true;
        }

        CHECK_ERROR(cudaDeviceSynchronize(), "device sync in computeBSSN");

        // Calculating memcopy times
        float eff_memcopy_time;
        if (is_bandwidth_calc){
            cudaEventElapsedTime(&eff_memcopy_time, start[0], end[0]);
            bssn::timer::t_memcopy.setTime(eff_memcopy_time/1000);
        }
        
        if (!is_bandwidth_calc) bssn::timer::t_memcopy_kernel.stop();

        // Release GPU memory
        bssn::timer::t_malloc_free.start();
        #include "bssnrhs_cuda_mdealloc.h"
        #include "bssnrhs_cuda_mdealloc_adv.h"

        for (int index=init_block; index<=current_block-1; index++){
            CHECK_ERROR(cudaFree(dev_var_in_array[index]), "dev_var_in_array[index] cudaFree");
            CHECK_ERROR(cudaFree(dev_var_out_array[index]), "dev_var_out_array[index] cudaFree");
        }
        bssn::timer::t_malloc_free.stop();
    }
    for (int index=0; index<num_streams; index++){
        CHECK_ERROR(cudaStreamDestroy(streams[index]), "cudaStream destruction");
    }
    return;
}

void GPU_pure_async_htod_dtoH_overlap(unsigned int numberOfLevels, Block * blkList, unsigned int lower_bound, unsigned int upper_bound, double ** var_in_array, double ** var_out_array, bool is_bandwidth_calc=0){ 
    CHECK_ERROR(cudaSetDevice(0), "cudaSetDevice in computeBSSN"); // Set the GPU that we are going to deal with

    // Creating cudaevents to measure the times 
    cudaEvent_t start[2], end[2];
    if (is_bandwidth_calc){
        for (int i=0; i<2; i++){
            cudaEventCreate(&start[i]);
            cudaEventCreate(&end[i]);
        }
    }

    // Creating cuda streams for the process
    int num_streams = 3;
    cudaStream_t stream;
    cudaStream_t streams[num_streams]; // usually steamCountToLevel[0] should be the max number of streams verify it before execute.
    for (int index=0; index<num_streams; index++){
        CHECK_ERROR(cudaStreamCreate(&streams[index]), "cudaStream creation");
    }

    // Check for available GPU memory
    size_t free_bytes, total_bytes;
    CHECK_ERROR(cudaMemGetInfo(&free_bytes, &total_bytes), "Available GPU memory checking failed");
    double GPU_capacity_buffer = 10;
    double GPUCapacity = 1.0*free_bytes/1024/1024 - GPU_capacity_buffer;
    std::cout << "Available GPU with buffer of " << GPU_capacity_buffer << ": " << GPUCapacity << " | Total GPU memory: " << total_bytes/1024/1024 << std::endl << std::endl;

    // Sort the data block list
    bssn::timer::t_sorting.start();
    mergeSort(blkList, 0, numberOfLevels-1); // O(nlog(n))
    bssn::timer::t_sorting.stop();

    // Calculate how much data block can be executed at once in GPU
    Block blk;
    int init_block = 0;
    int current_block = 0;
    int total_points = 0;
    int numberOfStreams = 0;
    double current_usage = 0;
    double fixed_usage = 0;
    double prev_usage = 0;
    double actual_usage = 0;
    double malloc_overhead = 3;

    while (current_block<numberOfLevels){
        current_usage=0;
        fixed_usage=0;
        init_block=current_block;

        while ((current_usage+fixed_usage<(GPUCapacity)) && (current_block<numberOfLevels)){
            prev_usage = current_usage+fixed_usage; // usage of previous iteration
            blk = blkList[current_block];
            total_points = blk.blkSize;
            numberOfStreams = 1;
            if (fixed_usage<numberOfStreams*(138+72)*total_points*sizeof(double)/1024/1024 + 210*malloc_overhead + (current_block-init_block)*malloc_overhead){
                fixed_usage = numberOfStreams*(138+72)*total_points*sizeof(double)/1024/1024 + 210*malloc_overhead + (current_block-init_block)*malloc_overhead; // 5*210 means allocation overhead
            }
            current_usage += (total_points*BSSN_NUM_VARS*sizeof(double)*2)/1024/1024;
            current_block++;
        }
        actual_usage = current_usage+fixed_usage;
        if (current_usage+fixed_usage>(GPUCapacity)){
            current_block--;
            if (init_block>current_block-1) {
                std::cout << "Required GPU memory = " << actual_usage << " Failed to allocate enough memory. Program terminated..." << std::endl;
                exit(0);
            }
            actual_usage = prev_usage;
        }

        // Display the set of blocks selected to process with their GPU usage
        std::cout << "start: " << init_block << " end: " << current_block-1 << "| usage: " << actual_usage << std::endl;

        // Allocating device memory to hold input and output
        double ** dev_var_in_array = new double*[numberOfLevels];
        double ** dev_var_out_array = new double*[numberOfLevels];

        int largest_intermediate_array = 0;
        for (int index=init_block; index<=current_block-1; index++){
            blk = blkList[index];
            numberOfStreams = 1;

            if (largest_intermediate_array<blk.blkSize*numberOfStreams){
                largest_intermediate_array = blk.blkSize*numberOfStreams;
            }
            bssn::timer::t_malloc_free.start();
            CHECK_ERROR(cudaMalloc((void**)&dev_var_in_array[index], blk.blkSize*BSSN_NUM_VARS*sizeof(double)), "dev_var_in_array[index]");
            CHECK_ERROR(cudaMalloc((void**)&dev_var_out_array[index], blk.blkSize*BSSN_NUM_VARS*sizeof(double)), "dev_var_out_array[index]");
            bssn::timer::t_malloc_free.stop();
        }
        
        // Allocation intermediate arrays
        bssn::timer::t_malloc_free.start();
        int size = largest_intermediate_array * sizeof(double);
        #include "bssnrhs_cuda_variable_malloc.h"
        #include "bssnrhs_cuda_variable_malloc_adv.h"
        #include "bssnrhs_cuda_malloc.h"
        #include "bssnrhs_cuda_malloc_adv.h"
        bssn::timer::t_malloc_free.stop();
        
        // Start block processing
        if (!is_bandwidth_calc) bssn::timer::t_memcopy_kernel.start();
        int streamIndex = 0;
        int unzip_dof;
        unsigned int sz[3];
        double ptmin[3], ptmax[3];
        unsigned int bflag;
        double dx, dy, dz;
        bool isAsyncStarted = false;
        for(int index=init_block; index<=current_block-1; index++) {
            blk=blkList[index];

            // Configure the block specific values
            unzip_dof=blk.blkSize;

            sz[0]=blk.node1D_x; 
            sz[1]=blk.node1D_y;
            sz[2]=blk.node1D_z;

            bflag=0;

            dx=0.1;
            dy=0.1;
            dz=0.1;

            ptmin[0]=0.0;
            ptmin[1]=0.0;
            ptmin[2]=0.0;

            ptmax[0]=1.0;
            ptmax[1]=1.0;
            ptmax[2]=1.0;

            // Starting async process

            // Sync any remaining process
            std::cout << "GPU - Count: " << std::setw(3) << index << " - Block no: " << std::setw(3) << blk.block_no << " - Bock level: " << std::setw(1) << blk.blkLevel << " - Block size: " << blk.blkSize << std::endl;

            if (is_bandwidth_calc && index==init_block) cudaEventRecord(start[0], streams[(index)%3]);
            CHECK_ERROR(cudaMemcpyAsync(dev_var_in_array[index], var_in_array[blk.block_no], BSSN_NUM_VARS*unzip_dof*sizeof(double), cudaMemcpyHostToDevice, streams[(index)%3]), "dev_var_in_array[index] cudaMemcpyHostToDevice");

            if (isAsyncStarted){
                cudaStreamSynchronize(streams[(index+2)%3]);
            }

            if (!is_bandwidth_calc){
                cuda_bssnrhs(dev_var_out_array[index], dev_var_in_array[index], unzip_dof, ptmin, ptmax, sz, bflag, streams[(index)%3],
                    #include "list_of_args_per_blk.h"
                );
            }

            if (isAsyncStarted){
                CHECK_ERROR(cudaMemcpyAsync(var_out_array[blkList[index-1].block_no], dev_var_out_array[index-1], BSSN_NUM_VARS*blkList[index-1].blkSize*sizeof(double), cudaMemcpyDeviceToHost, streams[(index+2)%3]), "dev_var_out_array[index] cudaMemcpyDeviceToHost");
            }

            //handle last DtoH memcopy
            if(index==current_block-1){
                CHECK_ERROR(cudaMemcpyAsync(var_out_array[blk.block_no], dev_var_out_array[index], BSSN_NUM_VARS*unzip_dof*sizeof(double), cudaMemcpyDeviceToHost, streams[(index)%3]), "dev_var_out_array[index] cudaMemcpyDeviceToHost");
            }

            if (is_bandwidth_calc && index==current_block-1) cudaEventRecord(end[0], streams[(index)%3]);

            if (!isAsyncStarted) isAsyncStarted=true;
        }

        CHECK_ERROR(cudaDeviceSynchronize(), "device sync in computeBSSN");

        // Calculating memcopy times
        float eff_memcopy_time;
        if (is_bandwidth_calc){
            cudaEventElapsedTime(&eff_memcopy_time, start[0], end[0]);
            bssn::timer::t_memcopy.setTime(eff_memcopy_time/1000);
        }
        
        if (!is_bandwidth_calc) bssn::timer::t_memcopy_kernel.stop();

        // Release GPU memory
        bssn::timer::t_malloc_free.start();
        #include "bssnrhs_cuda_mdealloc.h"
        #include "bssnrhs_cuda_mdealloc_adv.h"

        for (int index=init_block; index<=current_block-1; index++){
            CHECK_ERROR(cudaFree(dev_var_in_array[index]), "dev_var_in_array[index] cudaFree");
            CHECK_ERROR(cudaFree(dev_var_out_array[index]), "dev_var_out_array[index] cudaFree");
        }
        bssn::timer::t_malloc_free.stop();
    }
    for (int index=0; index<num_streams; index++){
        CHECK_ERROR(cudaStreamDestroy(streams[index]), "cudaStream destruction");
    }
    return;
}

void CPU_sequence(unsigned int numberOfLevels, Block * blkList, double ** var_in, double ** var_out){

    double ptmin[3], ptmax[3];
    unsigned int sz[3];
    unsigned int bflag;
    unsigned int offset;
    double dx, dy, dz;

    for(unsigned int blk=0; blk<numberOfLevels; blk++){

        offset=blkList[blk].offset;
        sz[0]=blkList[blk].node1D_x; 
        sz[1]=blkList[blk].node1D_y;
        sz[2]=blkList[blk].node1D_z;

        bflag=0; // indicates if the block is bdy block.

        dx=0.1;
        dy=0.1;
        dz=0.1;

        ptmin[0]=0.0;
        ptmin[1]=0.0;
        ptmin[2]=0.0;

        ptmax[0]=1.0;
        ptmax[1]=1.0;
        ptmax[2]=1.0;

        std::cout << "CPU - Count: " << std::setw(3) << blk <<  " - Block no: " << std::setw(3) << blkList[blk].block_no << " - Bock level: " << std::setw(1) << blkList[blk].blkLevel << " - Block size: " << blkList[blk].blkSize << std::endl;
        
        bssnrhs(var_out, (const double **)var_in, offset, ptmin, ptmax, sz, bflag);
    }
}

int main (int argc, char** argv){
    /**
     *
     * parameters:
     * blk_lb: block element 1d lower bound (int)
     * blk_up: block element 1d upper bound (int) (blk_up>=blk_lb)
     * numblks : number of blocks needed for each block sizes. (total_blks= (blk_up-blk_lb+1)*numblks)
     *
     * */

    if(argc<8){
        std::cout<<"Usage: " << argv[0] << " mean(double) std(double) numberOfBlocks(int) lowerLevel(int) upperLevel(int) isRandom(1/0) isTest(1/0)" <<std::endl;
        exit(0);
    }

    double mean = atoi(argv[1]);
    double std = atoi(argv[2]);
    unsigned int numberOfLevels = atoi(argv[3]);
    unsigned int lower_bound = atoi(argv[4]);
    unsigned int upper_bound = atoi(argv[5]);
    bool isRandom = atoi(argv[6]);
    bool isTest = atoi(argv[7]);

    bssn::timer::total_runtime.start();

    Block * blkList = new Block[numberOfLevels];
    double ** var_in_array = new double*[numberOfLevels];
    double ** var_out_array = new double*[numberOfLevels];

    double ** var_in;
    double ** var_out;
    if (isTest){
        var_in = new double*[BSSN_NUM_VARS];
        var_out = new double*[BSSN_NUM_VARS];
        data_generation_blockwise_and_bssn_var_wise_mixed(mean, std, numberOfLevels, lower_bound, upper_bound, isRandom, blkList, var_in_array, var_out_array, var_in, var_out);
    }else{
        data_generation_blockwise_mixed(mean, std, numberOfLevels, lower_bound, upper_bound, isRandom, blkList, var_in_array, var_out_array);
    }
    
    #include "rhs_cuda.h"

    #if bandwidth
        // Calc bandwidth
        #if parallelized
            GPU_parallelized(numberOfLevels, blkList, lower_bound, upper_bound, var_in_array, var_out_array, 1);
        #endif
        #if parallel_async_hybrid
            GPU_parallelized_async_hybrid(numberOfLevels, blkList, lower_bound, upper_bound, var_in_array, var_out_array, 1);
        #endif
        #if pure_async
            GPU_pure_async(numberOfLevels, blkList, lower_bound, upper_bound, var_in_array, var_out_array, 1);
        #endif
        #if pure_async_htod_dtoH_overlap
            GPU_pure_async_htod_dtoH_overlap(numberOfLevels, blkList, lower_bound, upper_bound, var_in_array, var_out_array, 1);
        #endif
    #endif

    bssn::timer::t_gpu_runtime.start();
    #if parallelized
        GPU_parallelized(numberOfLevels, blkList, lower_bound, upper_bound, var_in_array, var_out_array);
    #endif
    #if parallel_async_hybrid
        GPU_parallelized_async_hybrid(numberOfLevels, blkList, lower_bound, upper_bound, var_in_array, var_out_array);
    #endif
    #if pure_async
        GPU_pure_async(numberOfLevels, blkList, lower_bound, upper_bound, var_in_array, var_out_array);
    #endif
    #if pure_async_htod_dtoH_overlap
        GPU_pure_async_htod_dtoH_overlap(numberOfLevels, blkList, lower_bound, upper_bound, var_in_array, var_out_array);
    #endif
    bssn::timer::t_gpu_runtime.stop();

    std::cout << "" << std::endl;

    if (isTest){
        #include "rhs.h"
        bssn::timer::t_cpu_runtime.start();
        CPU_sequence(numberOfLevels, blkList, var_in, var_out);
        bssn::timer::t_cpu_runtime.stop();

        // Verify outputs
        for (int blk=0; blk<numberOfLevels; blk++){
            for(int bssn_var=0; bssn_var<BSSN_NUM_VARS; bssn_var++){

                int sizeofBlock = blkList[blk].blkSize;

                for (int pointInd=0; pointInd<sizeofBlock; pointInd++){
                    double diff = var_out_array[blkList[blk].block_no][bssn_var*sizeofBlock+pointInd] - var_out[bssn_var][blkList[blk].offset+pointInd];
                    if (fabs(diff)>threshold){
                        const char separator    = ' ';
                        const int nameWidth     = 6;
                        const int numWidth      = NUM_DIGITS+10;

                        std::cout << std::left << std::setw(nameWidth) << setfill(separator) << "GPU: ";
                        std::cout <<std::setprecision(NUM_DIGITS)<< std::left << std::setw(numWidth) << setfill(separator)  << var_out_array[blkList[blk].block_no][bssn_var*sizeofBlock+pointInd];

                        std::cout << std::left << std::setw(nameWidth) << setfill(separator) << "CPU: ";
                        std::cout <<std::setprecision(NUM_DIGITS)<< std::left << std::setw(numWidth) << setfill(separator)  << var_out[bssn_var][blkList[blk].offset+pointInd];

                        std::cout << std::left << std::setw(nameWidth) << setfill(separator) << "DIFF: ";
                        std::cout <<std::setprecision(NUM_DIGITS)<< std::left << std::setw(numWidth) << setfill(separator)  << diff << std::endl;
                        exit(0);
                    }
                }
            }
        }

        for (int var=0; var<BSSN_NUM_VARS; var++){
            delete [] var_in[var];
            delete [] var_out[var];
        }
    }

    //Free host memory
    for (int blk=0; blk<numberOfLevels; blk++){
        CHECK_ERROR(cudaFreeHost(var_in_array[blk]), "free host memory");
        CHECK_ERROR(cudaFreeHost(var_out_array[blk]), "free host memory");
    }

    bssn::timer::total_runtime.stop();
    bssn::timer::profileInfo();
    return 0;
}
