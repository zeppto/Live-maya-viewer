#ifndef MayaViewer_H_
#define MayaViewer_H_

#include "gameplay.h"
#include "ComLib.h"
#include <iostream>
#include <vector>

//filformat
struct M_Heder
{
	int meshCount = 0;
	int materialCount = 0;
	int cameraCount = 0;
	int lightCount = 0;
};

//the number of changes
struct M_SendHeder
{
	int newMesh = 0;
	int newMaterial = 0;
	int newCamera = 0;
	//int newLight = 0;
	int nameChange = 0;
	int meshChange = 0;
	int materialChange = 0;
	int cameraChange = 0;
	//int lightChange = 0;
};

struct M_NewName
{
	char oldName[50];
	char newName[50];
};

struct M_Trasform
{
	float TransformMatrix[4][4];
};

struct M_Mesh
{
	char name[50];
	int verticeCount = 0;
	int indexCount = 0;
	int materialID = -1;
	M_Trasform meshTransform;
};

struct M_VertexIndex
{
	int index;
};

struct M_Vertice
{
	float pos[3];
	float normal[3];
	float uv[2];
	/*float tangent[3];
	float bitangent[3];*/
};

//struct M_MeshVertexIndex
//{
//	std::vector<VertexIndex> meshVtxIndex;
//};
//
//struct M_MeshVertices
//{
//	std::vector<Vertice> meshVtxs;
//};

struct M_Material
{
	char name[50];
	float color[4];
	//bool mapsDSN[3];
	char diffuseMap[200] = "";
	//char specularMap[100];
	//char normalMap[100];
};

struct M_Camera
{
	int id;
	bool isOrtho;
	//float pos[3];
	//float upVec[3];
	//float direction[3];
	float aspectRatio;
	float FOV, Ner, Far;
	float oWhith;
	M_Trasform cameraTransform;
};

struct M_Light
{
	char type[50];
	//float pos[3];
	int intensety;
	float color[4];
	M_Trasform lightTransform;
};

using namespace gameplay;

/**
 * Main game class.
 */
class MayaViewer: public Game
{
public:

    /**
     * Constructor.
     */
    MayaViewer();

    /**
     * @see Game::keyEvent
     */
	void keyEvent(Keyboard::KeyEvent evt, int key);
	
    /**
     * @see Game::touchEvent
     */
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

	// mouse events
	bool mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta);


protected:

    /**
     * @see Game::initialize
     */
    void initialize();

    /**
     * @see Game::finalize
     */
    void finalize();

    /**
     * @see Game::update
     */
    void update(float elapsedTime);

    /**
     * @see Game::render
     */
    void render(float elapsedTime);

private:

    /**
     * Draws the scene each frame.
     */
    bool drawScene(Node* node);


	Mesh* createCubeMesh(float size = 1.0f);
	Material* createMaterial();

	Mesh* setUpNewMesh(char* ptr, int* offset);
	void setUpNewMaterial(char* ptr, int* offset);
	void setUpNewCamera(char* ptr, int* offset);
	void updateCamera(int id, char* ptr, int* offset);
	void updateMesh(int id, char* ptr, int* offset);
	void updateMaterial(int id, char* ptr, int* offset);
	void nameChange(char* ptr, int* offset);
	void setUpModel(Model* model, Material* mat, Texture::Sampler* sampler, M_Mesh mesh);
	void addNewModel(Model* model, M_Mesh mesh);
	void changeModel(Model* model, M_Mesh mesh);

	//void getWorldMatrixValus(M_Trasform curentTransform, Vector3* translate, Quaternion* rotation, Vector3* scale);
	void getWorldMatrixValus(M_Trasform curentTransform, Node* curentNode);

	ComLib myComLib = ComLib("MayaKey", 100 * 1 << 20, ComLib::TYPE::CONSUMER);
	M_SendHeder m_SendHeder;
	M_Heder m_Heder;
	std::vector<M_Mesh> m_Meshes;
	std::vector<M_Material> m_Materials;
	std::vector<M_Camera> m_Cameras;



    Scene* _scene;
    bool _wireframe;
};

#endif
