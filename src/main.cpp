// HEADERS E BIBLIOTECAS
#include <bits/stdc++.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Bibliotecas OpenGL
#include <glad/glad.h>  // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h> // Criação de janelas do sistema operacional

// Biblioteca GLM
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Bibliotecas .obj .mtl
#include <tiny_obj_loader.h>
#include <stb_image.h>

// Headers locais
#include "utils.h"
#include "matrices.h"
#include "collisions.h"
#include "util_functions.h"

// ESTRUTURAS DE DADOS

// Modelo do arquivo .obj
struct ObjModel
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    // Leitura arquivo .obj
    ObjModel(const char *filename, const char *basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i + 1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                        filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};

// Dados de renderização de cada objeto na cena
struct SceneObject
{
    std::string name;              // Nome do objeto
    size_t first_index;            // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t num_indices;            // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum rendering_mode;         // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3 bbox_min;            // Axis-Aligned Bounding Box do objeto
    glm::vec3 bbox_max;
};

Player player;

glm::vec3 local_bunny_min = glm::vec3(-0.3f, 0.0f, -0.3f);
glm::vec3 local_bunny_max = glm::vec3(0.3f, 1.1f, 0.3f);
float sphere_radius = 1.2f;

// DECLARAÇÃO DAS FUNÇÕES

// Funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4 &M);

// Funções usadas mna main
void BuildTrianglesAndAddToVirtualScene(ObjModel *); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel *model);                // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles();                         // Carrega os shaders de vértice e fragmento, criando um programa de GPU
GLuint LoadTextureImage(const char *filename);       // Função que carrega imagens de textura
std::map<std::string, GLuint> LoadTexturesFromObjModel(ObjModel *model, std::string base_path);
void DrawVirtualObject(const char *object_name); // Desenha um objeto armazenado em g_VirtualScene
void DrawVirtualObjectMtl(ObjModel *model, std::map<std::string, GLuint> textures_name_to_id, int obj_num);
GLuint LoadShader_Vertex(const char *filename);                              // Carrega um vertex shader
GLuint LoadShader_Fragment(const char *filename);                            // Carrega um fragment shader
void LoadShader(const char *filename, GLuint shader_id);                     // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel *);                                          // Função para debugging

// Funções de renderização de texto na janela (definidas em  "textrendering.cpp")
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow *window);
float TextRendering_CharWidth(GLFWwindow *window);
void TextRendering_PrintString(GLFWwindow *window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow *window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow *window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow *window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções relacionadas à janela OpenGL
void TextRendering_ShowModelViewProjection(GLFWwindow *window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow *window);
void TextRendering_ShowProjection(GLFWwindow *window);
void TextRendering_ShowFramesPerSecond(GLFWwindow *window);

// Funções callback para comunicação com o sistema operacional e interação do usuário
void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
void ErrorCallback(int error, const char *description);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos);
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset);

// VARIÁVEIS GLOBAIS

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4> g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false;  // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f;    // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;      // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

bool start_click = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_has_texture_uniform;             // NOVA: Uniform para indicar se há textura
GLint g_material_Ka_uniform;             // NOVA: Uniform para material.ambient
GLint g_material_Kd_uniform;             // NOVA: Uniform para material.diffuse
GLint g_material_Ks_uniform;             // NOVA: Uniform para material.specular
GLint g_material_Ns_uniform;             // NOVA: Uniform para material.shininess
GLint g_material_d_uniform;              // NOVA: Uniform para material.dissolve
GLint g_diffuse_texture_sampler_uniform; // NOVA: Uniform para o sampler da textura difusa

// vetores da câmera
glm::vec3 camera_pos = glm::vec3(0.0f, -0.5f, 5.0f);
glm::vec3 camera_front = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up = glm::vec3(0.0f, 1.0f, 0.0f);

// mouse sensitivity
float sensitivity = 0.1f;

// movimentação
float last_time = 0.0f;
float time_diff = 0.0f;

// velocidade do jogador
float normal_speed = 10.5f;
float run_speed = 15.5f;

// Pontos de controle para a curva de Bézier
glm::vec3 bezier_p0 = glm::vec3(1.5f, 0.0f, 0.0f);
glm::vec3 bezier_p1 = glm::vec3(0.75f, 0.0f, 1.5f);
glm::vec3 bezier_p2 = glm::vec3(-0.75f, 0.0f, 1.5f);
glm::vec3 bezier_p3 = glm::vec3(-1.5f, 0.0f, 0.0f);

// Controles do comportamento durante a curva
float bezier_t = 0.0f;
float bezier_speed = 0.15f;
int bezier_direction = 1;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;
GLuint g_CrosshairShaderID = 0;
GLuint g_CrosshairVao = 0;

std::string interactive_text = "";
float interactive_timer = 0.0f;
bool is_e_pressed = false;

// MAIN

int main(int argc, char *argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    // Create full screen window
    GLFWwindow *window;
    window = glfwCreateWindow(mode->width, mode->height, "Aline e Mari", monitor, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *glversion = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel roommodel("../../data/room/V7NCMVM2CESW8Q4IP1OHYDUES.obj");
    ComputeNormals(&roommodel);
    BuildTrianglesAndAddToVirtualScene(&roommodel);
    std::map<std::string, GLuint> room_textures = LoadTexturesFromObjModel(&roommodel, "../../data/room/");

    ObjModel floormodel("../../data/floor/plane.obj");
    ComputeNormals(&floormodel);
    BuildTrianglesAndAddToVirtualScene(&floormodel);

    ObjModel bedmodel("../../data/bed/RFSBI1TUMBM1R2T06LF7JZ3LC.obj");
    ComputeNormals(&bedmodel);
    BuildTrianglesAndAddToVirtualScene(&bedmodel);
    std::map<std::string, GLuint> bed_textures = LoadTexturesFromObjModel(&bedmodel, "../../data/bed/");

    ObjModel stovemodel("../../data/stove/75VMC3RPY8QVUEA16B08ZZLW1.obj");
    ComputeNormals(&stovemodel);
    BuildTrianglesAndAddToVirtualScene(&stovemodel);
    std::map<std::string, GLuint> stove_textures = LoadTexturesFromObjModel(&stovemodel, "../../data/stove/");

    ObjModel sofamodel("../../data/sofa/ACNAORMOA9N1GSJY0ZMQKTLCE.obj");
    ComputeNormals(&sofamodel);
    BuildTrianglesAndAddToVirtualScene(&sofamodel);
    std::map<std::string, GLuint> sofa_textures = LoadTexturesFromObjModel(&sofamodel, "../../data/sofa/");

    ObjModel doormodel("../../data/door/IBWGNOPUJHOS4JCGGWUZYBYR5.obj");
    ComputeNormals(&doormodel);
    BuildTrianglesAndAddToVirtualScene(&doormodel);
    std::map<std::string, GLuint> door_textures = LoadTexturesFromObjModel(&doormodel, "../../data/door/");

    ObjModel cabinet1model("../../data/cabinet1/LKB0Z15Y2JSWNIKSJZ9B8WCNY.obj");
    ComputeNormals(&cabinet1model);
    BuildTrianglesAndAddToVirtualScene(&cabinet1model);
    std::map<std::string, GLuint> cabinet1_textures = LoadTexturesFromObjModel(&cabinet1model, "../../data/cabinet1/");

    ObjModel tvmodel("../../data/tv/R471NYP16HGJGU9S7TBFXBO3E.obj");
    ComputeNormals(&tvmodel);
    BuildTrianglesAndAddToVirtualScene(&tvmodel);
    std::map<std::string, GLuint> tv_textures = LoadTexturesFromObjModel(&tvmodel, "../../data/tv/");

    ObjModel radiatormodel("../../data/radiator/LBIEJVJ5WQ38A21GOY8QEG7K4.obj");
    ComputeNormals(&radiatormodel);
    BuildTrianglesAndAddToVirtualScene(&radiatormodel);
    std::map<std::string, GLuint> radiator_textures = LoadTexturesFromObjModel(&radiatormodel, "../../data/radiator/");

    ObjModel ratmodel("../../data/rat/8LTPBJCTY0WWDFOHE0U5BMLHQ.obj");
    ComputeNormals(&ratmodel);
    BuildTrianglesAndAddToVirtualScene(&ratmodel);
    std::map<std::string, GLuint> rat_textures = LoadTexturesFromObjModel(&ratmodel, "../../data/rat/");

    ObjModel tablemodel("../../data/table/YZVB78847CT0OE8WDRU4I9Q9B.obj");
    ComputeNormals(&tablemodel);
    BuildTrianglesAndAddToVirtualScene(&tablemodel);
    std::map<std::string, GLuint> table_textures = LoadTexturesFromObjModel(&tablemodel, "../../data/table/");

    if (argc > 1)
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    player.position = glm::vec3(0.0f, 0.0f, 2.0f);
    player.front_vector = glm::vec3(0.0f, 0.0f, -1.0f);
    player.up_vector = glm::vec3(0.0f, 1.0f, 0.0f);
    player.local_collider = {
        glm::vec3(-0.3f, -1.0f, -0.3f), // min
        glm::vec3(0.3f, 0.8f, 0.3f)     // max
    };

    // FONTE: Chat gpt + gemini
    GLuint crosshair_vertex_shader_id = LoadShader_Vertex("../../src/crosshair_vertex.glsl");
    GLuint crosshair_fragment_shader_id = LoadShader_Fragment("../../src/crosshair_fragment.glsl");
    g_CrosshairShaderID = CreateGpuProgram(crosshair_vertex_shader_id, crosshair_fragment_shader_id);

    // 2. Define a geometria do crosshair (um pequeno quadrado)
    float crosshair_size = 0.005f; // Tamanho do ponto (0.01 = 1% da altura da tela). Aumente para um ponto maior.
    float crosshair_vertices[] = {
        // Primeiro triângulo do quadrado
        -crosshair_size, -crosshair_size, // Canto inferior esquerdo
        crosshair_size, -crosshair_size,  // Canto inferior direito
        crosshair_size, crosshair_size,   // Canto superior direito

        // Segundo triângulo do quadrado
        -crosshair_size, -crosshair_size, // Canto inferior esquerdo
        crosshair_size, crosshair_size,   // Canto superior direito
        -crosshair_size, crosshair_size   // Canto superior esquerdo
    };

    // 3. Cria o objeto (VAO) e envia os vértices para a GPU
    GLuint crosshair_vbo;
    glGenVertexArrays(1, &g_CrosshairVao);
    glGenBuffers(1, &crosshair_vbo);

    glBindVertexArray(g_CrosshairVao);
    glBindBuffer(GL_ARRAY_BUFFER, crosshair_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshair_vertices), crosshair_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

    glBindVertexArray(0); // Desvincula para segurança

    std::vector<CollidableObject> scene_collidables;

    // Matriz ROOM
    float room_scale1 = 20.0f;
    float room_scale2 = 15.0f;
    glm::vec3 room_position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::mat4 room_model_matrix = Matrix_Translate(room_position.x, room_position.y, room_position.z) * Matrix_Scale(room_scale1, room_scale2, room_scale1);

    float zmin = -20;
    float zmax = 20;
    float xmin = 20;
    float xmax = -20;

    CollidableObject parede_fundo;
    parede_fundo.shape_type = ShapeType::SHAPE_PLANE;
    parede_fundo.plane = {glm::vec3(0.0f, 0.0f, +1.0f), -zmin};
    parede_fundo.text = "";
    parede_fundo.is_interactive = false;

    CollidableObject parede_frente;
    parede_frente.shape_type = ShapeType::SHAPE_PLANE;
    parede_frente.plane = {glm::vec3(0.0f, 0.0f, -1.0f), zmax};
    parede_frente.text = "";
    parede_frente.is_interactive = false;

    CollidableObject parede_esquerda;
    parede_esquerda.shape_type = ShapeType::SHAPE_PLANE;
    parede_esquerda.plane = {glm::vec3(+1.0f, 0.0f, 0.0f), -xmin};
    parede_esquerda.text = "";
    parede_esquerda.is_interactive = false;

    CollidableObject parede_direita;
    parede_direita.shape_type = ShapeType::SHAPE_PLANE;
    parede_direita.plane = {glm::vec3(-1.0f, 0.0f, 0.0f), xmax};
    parede_direita.text = "";
    parede_direita.is_interactive = false;

    scene_collidables.push_back(parede_fundo);
    scene_collidables.push_back(parede_frente);
    scene_collidables.push_back(parede_esquerda);
    scene_collidables.push_back(parede_direita);

    // Matriz FLOOR
    float floor_scale = 30.0f;
    glm::vec3 floor_position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::mat4 floor_model_matrix = Matrix_Translate(0.0f, -9.0f, 0.0f) * Matrix_Scale(floor_scale, floor_scale, floor_scale);

    float floor_y = -9.0f + floor_scale;

    CollidableObject floor;
    floor.shape_type = ShapeType::SHAPE_PLANE;
    floor.plane = {glm::vec3(0.0f, 1.0f, 0.0f), -floor_y};
    floor.text = "";
    floor.is_interactive = false;
    scene_collidables.push_back(floor);

    // Matriz BED
    float bed_scale = 5.0f;
    glm::vec3 bed_position = glm::vec3(-13.0f, -6.0f, 15.0f);
    glm::mat4 bed_model_matrix = Matrix_Translate(bed_position.x, bed_position.y, bed_position.z) * Matrix_Scale(bed_scale, bed_scale, bed_scale);

    CollidableObject bed;
    bed.shape_type = ShapeType::SHAPE_AABB;
    bed.aabb.min = glm::vec3(-20.10f, -8.0f, 8.750f);
    bed.aabb.max = glm::vec3(-6.49f, 3.0f, 19.62f);
    bed.text = "cama";
    bed.is_interactive = true;
    scene_collidables.push_back(bed);

    // Matriz STOVE
    float stove_scale = 2.0f;
    glm::vec3 stove_position = glm::vec3(18.0f, -6.5f, -17.0f);
    glm::mat4 stove_model_matrix = Matrix_Translate(stove_position.x, stove_position.y, stove_position.z) * Matrix_Scale(stove_scale, stove_scale, stove_scale);

    CollidableObject stove;
    stove.shape_type = ShapeType::SHAPE_AABB;
    stove.aabb.min = glm::vec3(15.11f, -5.0f, -19.35f);
    stove.aabb.max = glm::vec3(19.5f, 5.0f, -13.61f);
    stove.text = "Voce desligou o fogao";
    stove.is_interactive = true;
    scene_collidables.push_back(stove);

    // Matriz SOFA
    float sofa_scale = 5.0f;
    glm::vec3 sofa_position = glm::vec3(12.0f, -6.5f, 5.0f);
    glm::mat4 sofa_model_matrix = Matrix_Translate(sofa_position.x, sofa_position.y, sofa_position.z) * Matrix_Scale(sofa_scale, sofa_scale, sofa_scale) * Matrix_Rotate_Y(1.55f);

    CollidableObject sofa;
    sofa.shape_type = ShapeType::SHAPE_AABB;
    sofa.aabb.min = glm::vec3(5.4f, -5.0f, 1.63f);
    sofa.aabb.max = glm::vec3(17.94f, 5.0f, 7.78f);
    sofa.text = "";
    sofa.is_interactive = false;
    scene_collidables.push_back(sofa);

    // Matriz DOOR
    float door_scale = 5.5f;
    glm::vec3 door_position = glm::vec3(-21.0f, -4.0f, -15.0f);
    glm::mat4 door_model_matrix = Matrix_Translate(door_position.x, door_position.y, door_position.z) * Matrix_Scale(door_scale, door_scale, door_scale) * Matrix_Rotate_Y(3.15f);

    CollidableObject door;
    door.shape_type = ShapeType::SHAPE_AABB;
    door.aabb.min = glm::vec3(-19.0f, -5.0f, -18.0f);
    door.aabb.max = glm::vec3(-19.0f, 5.0f, -12.0f);
    door.text = "Voce trancou a porta";
    door.is_interactive = true;
    scene_collidables.push_back(door);

    // Matriz CABINET1
    float cabinet1_scale = 2.5f;
    glm::vec3 cabinet1_position = glm::vec3(12.0f, -6.5f, 18.0f);
    glm::mat4 cabinet1_model_matrix = Matrix_Translate(cabinet1_position.x, cabinet1_position.y, cabinet1_position.z) * Matrix_Scale(cabinet1_scale, cabinet1_scale, cabinet1_scale) * Matrix_Rotate_Y(-1.60f);

    // Matriz TV
    float tv_scale = 1.2f;
    glm::vec3 tv_position = glm::vec3(12.0f, -2.8f, 18.5f);
    glm::mat4 tv_model_matrix = Matrix_Translate(tv_position.x, tv_position.y, tv_position.z) * Matrix_Scale(tv_scale, tv_scale, tv_scale) * Matrix_Rotate_Y(-1.60f);

    CollidableObject tv;
    tv.shape_type = ShapeType::SHAPE_AABB;
    tv.aabb.min = glm::vec3(9.5f, -5.0f, 15.4f);
    tv.aabb.max = glm::vec3(14.5f, 5.0f, 19.6f);
    tv.text = "";
    tv.is_interactive = false;
    scene_collidables.push_back(tv);

    // Matriz RADIATOR
    float radiator_scale = 2.0f;
    glm::vec3 radiator_position = glm::vec3(-20.0f, -7.0f, 5.0f);
    glm::mat4 radiator_model_matrix = Matrix_Translate(radiator_position.x, radiator_position.y, radiator_position.z) * Matrix_Scale(radiator_scale, radiator_scale, radiator_scale) * Matrix_Rotate_Y(-1.60f);

    CollidableObject radiator;
    radiator.shape_type = ShapeType::SHAPE_AABB;
    radiator.aabb.min = glm::vec3(-19.7f, -5.0f, 1.5f);
    radiator.aabb.max = glm::vec3(-18.0f, 5.0f, 7.0f);
    radiator.text = "Voce desligou o radiador";
    radiator.is_interactive = true;
    scene_collidables.push_back(radiator);

    // Matriz RAT
    float rat_scale = 1.2f;
    glm::vec3 rat_position = glm::vec3(4.0f, -4.5f, -3.0f);
    glm::mat4 rat_model_matrix = Matrix_Translate(rat_position.x, rat_position.y, rat_position.z) * Matrix_Scale(rat_scale, rat_scale, rat_scale) * Matrix_Rotate_Y(-1.4f);

    // Matriz TABLE
    float table_scale = 3.5f;
    glm::vec3 table_position = glm::vec3(4.0f, -7.5f, -4.0f);
    glm::mat4 table_model_matrix = Matrix_Translate(table_position.x, table_position.y, table_position.z) * Matrix_Scale(table_scale, table_scale, table_scale) * Matrix_Rotate_Y(-0.0f);

    CollidableObject table;
    table.shape_type = ShapeType::SHAPE_AABB;
    table.aabb.min = glm::vec3(0.6f, -5.0f, -9.0f);
    table.aabb.max = glm::vec3(6.8f, 5.0f, 0.3f);
    table.text = "Uma mesa com um rato em cima.";
    table.is_interactive = true;
    scene_collidables.push_back(table);

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        std::vector<Plane> room_planes;

        // Mantém o tempo constante
        float current_time = (float)glfwGetTime();
        time_diff = current_time - last_time;
        last_time = current_time;

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(g_GpuProgramID);

        UpdatePlayerPosition(window, time_diff, player, scene_collidables);

        printf("Player Position: X=%.2f, Y=%.2f, Z=%.2f\r",
               player.position.x, player.position.y, player.position.z);
        fflush(stdout); // Força a impressão imediata no terminal

        if (is_e_pressed)
        {
            bool is_player_asleep = false;
            std::string result_text = CheckRaycastFromCenter(player, scene_collidables, is_player_asleep);

            if (!result_text.empty())
            {
                interactive_text = result_text;
                interactive_timer = 2.0f;
            }
            is_e_pressed = false;
        }
        // Câmera livre
        glm::vec4 camera_position_c = glm::vec4(player.position, 1.0f);
        glm::vec4 camera_view_vector = glm::vec4(player.front_vector, 0.0f);
        glm::vec4 camera_up_vector = glm::vec4(player.up_vector, 0.0f);
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        float nearplane = -0.1f;   // Posição do "near plane"
        float farplane = -1000.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            float t = 1.5f * g_CameraDistance / 2.5f;
            float b = -t;
            float r = t * g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model;

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

#define ROOM 0
#define BED 1
#define FLOOR 2
#define STOVE 3
#define SOFA 4
#define DOOR 5
#define CABINET1 6
#define TV 7
#define RADIATOR 8
#define RAT 9
#define TABLE 10

        glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(room_model_matrix));
        DrawVirtualObjectMtl(&roommodel, room_textures, ROOM);

        glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(floor_model_matrix));
        glUniform1f(g_object_id_uniform, FLOOR);
        DrawVirtualObject("the_plane");

        glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(bed_model_matrix));
        DrawVirtualObjectMtl(&bedmodel, bed_textures, BED);

        glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(stove_model_matrix));
        DrawVirtualObjectMtl(&stovemodel, stove_textures, STOVE);

        glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(sofa_model_matrix));
        DrawVirtualObjectMtl(&sofamodel, sofa_textures, SOFA);

        glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(door_model_matrix));
        DrawVirtualObjectMtl(&doormodel, door_textures, DOOR);

        glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(cabinet1_model_matrix));
        DrawVirtualObjectMtl(&cabinet1model, cabinet1_textures, CABINET1);

        glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(tv_model_matrix));
        DrawVirtualObjectMtl(&tvmodel, tv_textures, CABINET1);

        glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(radiator_model_matrix));
        DrawVirtualObjectMtl(&radiatormodel, radiator_textures, RADIATOR);

        glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(rat_model_matrix));
        DrawVirtualObjectMtl(&ratmodel, rat_textures, RAT);

        glActiveTexture(GL_TEXTURE0);
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(table_model_matrix));
        DrawVirtualObjectMtl(&tablemodel, table_textures, TABLE);

        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        glDisable(GL_DEPTH_TEST); // Para o crosshair ficar sempre por cima

        if (interactive_timer > 0.0f)
        {
            interactive_timer -= time_diff;
            float x = -0.5f * interactive_text.length() * 0.05f;
            TextRendering_PrintString(window, interactive_text, x, -0.8f, 3.0f);
        }

        glUseProgram(g_CrosshairShaderID); // Ativa o shader do crosshair
        glBindVertexArray(g_CrosshairVao); // Ativa o objeto do crosshair

        // Desenha as 4 vértices como 2 linhas separadas
        // (Assumindo que você ainda tem 4 vértices no seu VAO)
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(0); // Desliga o VAO por segurança
        glEnable(GL_DEPTH_TEST);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
GLuint LoadTextureImage(const char *filename, int dummy_param)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if (data == NULL)
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id; // Samplers são geralmente vinculados uma vez e reutilizados, mas aqui é criado por textura.
                       // Para um controle mais eficiente, samplers podem ser globais ou gerenciados separadamente.
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Vinculamos a textura criada para configurá-la
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    // Note: A unidade de textura `textureunit` e `glActiveTexture` foram movidas.
    // O gerenciamento de `textureunit` deve ser feito pela função chamadora
    // (LoadTexturesFromObjModel) ou de forma global para alocação de unidades.
    // Aqui, apenas criamos e configuramos a textura e o sampler.

    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Após configurar a textura, podemos desvinculá-la para não afetar outras operações.
    glBindTexture(GL_TEXTURE_2D, 0); // Desvincula a textura atual
    // Sampler pode ser vinculado posteriormente na fase de desenho com glBindTextureUnit ou glBindSampler

    stbi_image_free(data);

    // Remove a linha `g_NumLoadedTextures += 1;` daqui.
    // Esta variável deve ser gerenciada pela função que carrega múltiplas texturas
    // e aloca unidades de textura, como `LoadTexturesFromObjModel` ou um gerenciador de texturas.

    return texture_id; // Retorna o ID da textura gerado
}
// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char *object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void *)(g_VirtualScene[object_name].first_index * sizeof(GLuint)));

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

void DrawVirtualObjectMtl(ObjModel *model, std::map<std::string, GLuint> textures_name_to_id, int obj_num)
{
    // NOVO: Use size_t para o contador do loop para evitar o warning de signedness
    for (size_t i = 0; i < model->shapes.size(); i++)
    {
        const auto &shape = model->shapes[i];
        int material_id = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[0];
        if (material_id < 0)
            continue;
        const auto &material = model->materials[material_id];

        // Ative a textura difusa do material, se existir
        if (!material.diffuse_texname.empty())
        {
            // Diz ao shader que TEMOS uma textura para usar
            glUniform1i(g_has_texture_uniform, 1);
            GLuint texture_id = textures_name_to_id[material.diffuse_texname];
            glActiveTexture(GL_TEXTURE0); // Ativa a unidade de textura 0
            glBindTexture(GL_TEXTURE_2D, texture_id);
            // Vincula o sampler uniform "diffuse_texture" à unidade de textura 0
            glUniform1i(g_diffuse_texture_sampler_uniform, 0);
        }
        else
        {
            // Diz ao shader que NÃO TEMOS uma textura, então ele deve usar apenas a cor
            glUniform1i(g_has_texture_uniform, 0);
        }

        // Envie parâmetros do material para o shader
        glUniform3fv(g_material_Ka_uniform, 1, material.ambient);
        glUniform3fv(g_material_Kd_uniform, 1, material.diffuse);
        glUniform3fv(g_material_Ks_uniform, 1, material.specular);
        glUniform1f(g_material_Ns_uniform, material.shininess);
        glUniform1f(g_material_d_uniform, material.dissolve);

        // Defina o object_id
        glUniform1i(g_object_id_uniform, obj_num);

        // Desenhe o shape
        DrawVirtualObject(shape.name.c_str());
    }
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if (g_GpuProgramID != 0)
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform = glGetUniformLocation(g_GpuProgramID, "model");           // Variável da matriz "model"
    g_view_uniform = glGetUniformLocation(g_GpuProgramID, "view");             // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform = glGetUniformLocation(g_GpuProgramID, "object_id");   // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform = glGetUniformLocation(g_GpuProgramID, "bbox_max");

    // NOVO: Inicialização dos novos uniforms
    g_has_texture_uniform = glGetUniformLocation(g_GpuProgramID, "has_texture");
    g_material_Ka_uniform = glGetUniformLocation(g_GpuProgramID, "material_Ka");
    g_material_Kd_uniform = glGetUniformLocation(g_GpuProgramID, "material_Kd");
    g_material_Ks_uniform = glGetUniformLocation(g_GpuProgramID, "material_Ks");
    g_material_Ns_uniform = glGetUniformLocation(g_GpuProgramID, "material_Ns");
    g_material_d_uniform = glGetUniformLocation(g_GpuProgramID, "material_d");
    g_diffuse_texture_sampler_uniform = glGetUniformLocation(g_GpuProgramID, "diffuse_texture"); // Certifique-se de que o nome no shader é "diffuse_texture"
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4 &M)
{
    if (g_MatrixStack.empty())
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel *model)
{
    if (!model->attrib.normals.empty())
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4 vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
                const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx, vy, vz, 1.0);
            }

            const glm::vec4 a = vertices[0];
            const glm::vec4 b = vertices[1];
            const glm::vec4 c = vertices[2];

            const glm::vec4 n = crossproduct(b - a, c - a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3 * triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize(3 * num_vertices);

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3 * i + 0] = n.x;
        model->attrib.normals[3 * i + 1] = n.y;
        model->attrib.normals[3 * i + 2] = n.z;
    }
}

// Carrega arquivos de textura de cada objeto
std::map<std::string, GLuint> LoadTexturesFromObjModel(ObjModel *model, std::string base_path)
{
    std::map<std::string, GLuint> texture_name_to_id;

    for (const auto &material : model->materials)
    {
        if (!material.diffuse_texname.empty() && texture_name_to_id.count(material.diffuse_texname) == 0)
        {
            std::string texpath = base_path + material.diffuse_texname;
            GLuint loaded_texture_id = LoadTextureImage(texpath.c_str(), 0);
            texture_name_to_id[material.diffuse_texname] = loaded_texture_id;
        }
    }
    return texture_name_to_id;
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel *model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float> model_coefficients;
    std::vector<float> normal_coefficients;
    std::vector<float> texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval, maxval, maxval);
        glm::vec3 bbox_max = glm::vec3(minval, minval, minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];

                indices.push_back(first_index + 3 * triangle + vertex);

                const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];
                // printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back(vx);   // X
                model_coefficients.push_back(vy);   // Y
                model_coefficients.push_back(vz);   // Z
                model_coefficients.push_back(1.0f); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if (idx.normal_index != -1)
                {
                    const float nx = model->attrib.normals[3 * idx.normal_index + 0];
                    const float ny = model->attrib.normals[3 * idx.normal_index + 1];
                    const float nz = model->attrib.normals[3 * idx.normal_index + 2];
                    normal_coefficients.push_back(nx);   // X
                    normal_coefficients.push_back(ny);   // Y
                    normal_coefficients.push_back(nz);   // Z
                    normal_coefficients.push_back(0.0f); // W
                }

                if (idx.texcoord_index != -1)
                {
                    const float u = model->attrib.texcoords[2 * idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2 * idx.texcoord_index + 1];
                    texture_coefficients.push_back(u);
                    texture_coefficients.push_back(v);
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name = model->shapes[shape].name;
        theobject.first_index = first_index;                  // Primeiro índice
        theobject.num_indices = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;              // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0;            // "(location = 0)" em "shader_vertex.glsl"
    GLint number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!normal_coefficients.empty())
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1;             // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (!texture_coefficients.empty())
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2;             // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char *filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char *filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char *filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try
    {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar *shader_string = str.c_str();
    const GLint shader_string_length = static_cast<GLint>(str.length());

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar *log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if (log_length != 0)
    {
        std::string output;

        if (!compiled_ok)
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete[] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if (linked_ok == GL_FALSE)
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar *log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete[] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow *window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    // Needs to be static so it doesnt "teleports" a little.
    // WARN: static variables were chosen after asking chat-GPT for help
    static float horizontal = -90.0f; // - Z, since we use right hand
    static float vertical = 0.0f;
    static float x_pos = 400.0f; // 800 x 800  / 2
    static float y_pos = 400.0f;

    // Mouse movement
    float xoffset = (float)xpos - x_pos;
    float yoffset = y_pos - (float)ypos;

    // Update last position
    x_pos = (float)xpos;
    y_pos = (float)ypos;

    // Apply sensibility
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Update angles
    horizontal += xoffset;
    vertical += yoffset;

    if (vertical > 89.0f) // Prevents camera from doing a sharp turn
        vertical = 89.0f;
    if (vertical < -89.0f)
        vertical = -89.0f;

    // Creates direction vector based on cursor movement
    glm::vec3 direction;
    direction.x = cos(glm::radians(horizontal)) * cos(glm::radians(vertical));
    direction.y = sin(glm::radians(vertical));
    direction.z = sin(glm::radians(horizontal)) * cos(glm::radians(vertical));
    player.front_vector = glm::normalize(direction);
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f * yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mod)
{
    // ======================
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ======================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    bool is_player_asleep = false;
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        is_e_pressed = true;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        g_ForearmAngleX = 0.0f;
        g_ForearmAngleZ = 0.0f;
        g_TorsoPositionX = 0.0f;
        g_TorsoPositionY = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout, "Shaders recarregados!\n");
        fflush(stdout);
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char *description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow *window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model)
{
    if (!g_ShowInfoText)
        return;

    glm::vec4 p_world = model * p_model;
    glm::vec4 p_camera = view * p_world;
    glm::vec4 p_clip = projection * p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f - pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f - 2 * pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f - 6 * pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f - 7 * pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f - 8 * pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f - 9 * pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f - 10 * pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f - 14 * pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f - 15 * pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f - 16 * pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f - 17 * pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f - 18 * pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2(0, 0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x) / (b.x - a.x), 0.0f, 0.0f, (b.x * p.x - a.x * q.x) / (b.x - a.x),
        0.0f, (q.y - p.y) / (b.y - a.y), 0.0f, (b.y * p.y - a.y * q.y) / (b.y - a.y),
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f - 22 * pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f - 23 * pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f - 24 * pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f - 25 * pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f - 26 * pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow *window)
{
    if (!g_ShowInfoText)
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f + pad / 10, -1.0f + 2 * pad / 10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow *window)
{
    if (!g_ShowInfoText)
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if (g_UsePerspectiveProjection)
        TextRendering_PrintString(window, "Perspective", 1.0f - 13 * charwidth, -1.0f + 2 * lineheight / 10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f - 13 * charwidth, -1.0f + 2 * lineheight / 10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow *window)
{
    if (!g_ShowInfoText)
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int ellapsed_frames = 0;
    static char buffer[20] = "?? fps";
    static int numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if (ellapsed_seconds > 1.0f)
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f - (numchars + 1) * charwidth, 1.0f - lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel *model)
{
    const tinyobj::attrib_t &attrib = model->attrib;
    const std::vector<tinyobj::shape_t> &shapes = model->shapes;
    const std::vector<tinyobj::material_t> &materials = model->materials;

    printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
    printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
    printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
    printf("# of shapes    : %d\n", (int)shapes.size());
    printf("# of materials : %d\n", (int)materials.size());

    for (size_t v = 0; v < attrib.vertices.size() / 3; v++)
    {
        printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.vertices[3 * v + 0]),
               static_cast<const double>(attrib.vertices[3 * v + 1]),
               static_cast<const double>(attrib.vertices[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.normals.size() / 3; v++)
    {
        printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.normals[3 * v + 0]),
               static_cast<const double>(attrib.normals[3 * v + 1]),
               static_cast<const double>(attrib.normals[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.texcoords.size() / 2; v++)
    {
        printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
               static_cast<const double>(attrib.texcoords[2 * v + 0]),
               static_cast<const double>(attrib.texcoords[2 * v + 1]));
    }

    // For each shape
    for (size_t i = 0; i < shapes.size(); i++)
    {
        printf("shape[%ld].name = %s\n", static_cast<long>(i),
               shapes[i].name.c_str());
        printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.indices.size()));

        size_t index_offset = 0;

        assert(shapes[i].mesh.num_face_vertices.size() ==
               shapes[i].mesh.material_ids.size());

        printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

        // For each face
        for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++)
        {
            size_t fnum = shapes[i].mesh.num_face_vertices[f];

            printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
                   static_cast<unsigned long>(fnum));

            // For each vertex in the face
            for (size_t v = 0; v < fnum; v++)
            {
                tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
                printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
                       static_cast<long>(v), idx.vertex_index, idx.normal_index,
                       idx.texcoord_index);
            }

            printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
                   shapes[i].mesh.material_ids[f]);

            index_offset += fnum;
        }

        printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
               static_cast<unsigned long>(shapes[i].mesh.tags.size()));
        for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++)
        {
            printf("  tag[%ld] = %s ", static_cast<long>(t),
                   shapes[i].mesh.tags[t].name.c_str());
            printf(" ints: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j)
            {
                printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
                if (j < (shapes[i].mesh.tags[t].intValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");

            printf(" floats: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j)
            {
                printf("%f", static_cast<const double>(
                                 shapes[i].mesh.tags[t].floatValues[j]));
                if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");

            printf(" strings: [");
            for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j)
            {
                printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
                if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1))
                {
                    printf(", ");
                }
            }
            printf("]");
            printf("\n");
        }
    }

    for (size_t i = 0; i < materials.size(); i++)
    {
        printf("material[%ld].name = %s\n", static_cast<long>(i),
               materials[i].name.c_str());
        printf("  material.Ka = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].ambient[0]),
               static_cast<const double>(materials[i].ambient[1]),
               static_cast<const double>(materials[i].ambient[2]));
        printf("  material.Kd = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].diffuse[0]),
               static_cast<const double>(materials[i].diffuse[1]),
               static_cast<const double>(materials[i].diffuse[2]));
        printf("  material.Ks = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].specular[0]),
               static_cast<const double>(materials[i].specular[1]),
               static_cast<const double>(materials[i].specular[2]));
        printf("  material.Tr = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].transmittance[0]),
               static_cast<const double>(materials[i].transmittance[1]),
               static_cast<const double>(materials[i].transmittance[2]));
        printf("  material.Ke = (%f, %f ,%f)\n",
               static_cast<const double>(materials[i].emission[0]),
               static_cast<const double>(materials[i].emission[1]),
               static_cast<const double>(materials[i].emission[2]));
        printf("  material.Ns = %f\n",
               static_cast<const double>(materials[i].shininess));
        printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
        printf("  material.dissolve = %f\n",
               static_cast<const double>(materials[i].dissolve));
        printf("  material.illum = %d\n", materials[i].illum);
        printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
        printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
        printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
        printf("  material.map_Ns = %s\n",
               materials[i].specular_highlight_texname.c_str());
        printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
        printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
        printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
        printf("  <<PBR>>\n");
        printf("  material.Pr     = %f\n", materials[i].roughness);
        printf("  material.Pm     = %f\n", materials[i].metallic);
        printf("  material.Ps     = %f\n", materials[i].sheen);
        printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
        printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
        printf("  material.aniso  = %f\n", materials[i].anisotropy);
        printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
        printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
        printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
        printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
        printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
        printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
        std::map<std::string, std::string>::const_iterator it(
            materials[i].unknown_parameter.begin());
        std::map<std::string, std::string>::const_iterator itEnd(
            materials[i].unknown_parameter.end());

        for (; it != itEnd; it++)
        {
            printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
        }
        printf("\n");
    }
}
