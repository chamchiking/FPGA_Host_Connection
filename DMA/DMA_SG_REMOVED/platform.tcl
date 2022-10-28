# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct C:\Users\USER\DRAM_vitis_hls\DMA_SG_REMOVED\platform.tcl
# 
# OR launch xsct and run below command.
# source C:\Users\USER\DRAM_vitis_hls\DMA_SG_REMOVED\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {DMA_SG_REMOVED}\
-hw {C:\Users\USER\Downloads\nexys_big_mem_baud\DMA_SG_REMOVED.xsa}\
-out {C:/Users/USER/DRAM_vitis_hls}

platform write
domain create -name {standalone_microblaze_0} -display-name {standalone_microblaze_0} -os {standalone} -proc {microblaze_0} -runtime {cpp} -arch {32-bit} -support-app {empty_application}
platform generate -domains 
platform active {DMA_SG_REMOVED}
platform generate -quick
platform generate
