#pragma once


namespace resource::shader {

    extern const std::vector<unsigned char> triangle_vertex_geom;
    extern const std::vector<unsigned char> flat_frag;
    extern const std::vector<unsigned char> triangle_frag;
    extern const std::vector<unsigned char> main_vert;

}



static vk::UniqueShaderModule loadShader(vk::Device device, const std::vector<unsigned char>& data) {
    return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo{
            /*flags*/    {},
            /*codeSize*/ data.size(),
            /*pCode*/    (const uint32_t*) data.data()
    });
}