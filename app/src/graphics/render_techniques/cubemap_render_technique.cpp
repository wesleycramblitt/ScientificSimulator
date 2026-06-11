#include "graphics/render_techniques/cubemap_render_technique.h"

namespace exd {
namespace graphics {
namespace render_techniques {

void CubeMapRenderTechnique::bind() {

    cubemap_program_ = graphicsContext_.shader_manager.getOrLoad(
            "cubemap",
            "shaders/cubemap/cubemap.vert",
            "shaders/cubemap/cubemap.frag"
            );

    GL_CALL(glDepthFunc(GL_LEQUAL));
    GL_CALL(glDepthMask(GL_FALSE));
    GL_CALL(glDisable(GL_CULL_FACE));

    GL_CALL(glUseProgram(cubemap_program_));


}

void CubeMapRenderTechnique::draw(const Renderable& renderable) {
    const GLint u_skybox = glGetUniformLocation(cubemap_program_, "u_skybox");
    GLint u_view = glGetUniformLocation(cubemap_program_, "u_view");
    GLint u_proj = glGetUniformLocation(cubemap_program_, "u_proj");

    GL_CALL(glUniformMatrix4fv(u_view, 1, GL_FALSE, std::get<math::Mat4>(renderable.uniforms.at("u_view")).m));
    GL_CALL(glUniformMatrix4fv(u_proj, 1, GL_FALSE, std::get<math::Mat4>(renderable.uniforms.at("u_proj")).m));

    if (renderable.mesh_handle == 0 || renderable.texture_handle ==0 ) {
        //TODO error handling
        return;
    }

    graphicsContext_.texture_manager.bind(renderable.texture_handle);
    GL_CALL(glUniform1i(u_skybox, 0));

    const MeshGPU* mesh =  graphicsContext_.mesh_manager.bind(renderable.mesh_handle);

    GL_CALL(glDrawArrays(mesh->topology, 0, (GLsizei)mesh->vertex_count));
    GL_CALL(glBindVertexArray(0));
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));

}


void CubeMapRenderTechnique::unbind() {
    GL_CALL(glDepthFunc(GL_LESS));
    GL_CALL(glDepthMask(GL_TRUE));
    GL_CALL(glEnable(GL_CULL_FACE));
    GL_CALL(glUseProgram(0));
}

} // namespace render_techniques
} // namespace graphics
} // namespace exd
