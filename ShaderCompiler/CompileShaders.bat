glslangValidator.exe -V ../Data/Shaders/src/VulkanTest.vert
glslangValidator.exe -V ../Data/Shaders/src/VulkanTest.frag

MOVE frag.spv ../data/shaders/bin
MOVE vert.spv ../data/shaders/bin
pause