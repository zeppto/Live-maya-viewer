#include "MayaViewer.h"

// Declare our game instance
MayaViewer game;

int gModelCount = 10;
static bool gKeys[256] = {};
int gDeltaX;
int gDeltaY;
bool gMousePressed;

MayaViewer::MayaViewer()
    : _scene(NULL), _wireframe(false)
{
}

void MayaViewer::initialize()
{
	char* myMsg = 0;
	size_t myMsgSize = 0;
	bool msgResived = false;
	int offset = 0;
	while (!msgResived)
	{
		msgResived = myComLib.recv(myMsg, myMsgSize);
	}
	m_SendHeder = *(M_SendHeder*)myMsg;
	m_Heder.meshCount += m_SendHeder.newMesh;
	gModelCount = m_Heder.meshCount;
	m_Heder.materialCount += m_SendHeder.newMaterial;
	m_Heder.cameraCount += m_SendHeder.newCamera;
	myMsg += sizeof(M_SendHeder);
	//setUpMesh(myMsg);

	std::vector<Model*> models;
	std::vector<Material*> mats;
	std::vector<Texture::Sampler*> samplers;
	for (int i = 0; i < m_Heder.meshCount; i++)
	{
		myMsg += offset;
		models.push_back(Model::create(setUpNewMesh(myMsg, &offset)));
	}
	for (int i = 0; i < m_Heder.materialCount; i++)
	{
		myMsg += offset;
		setUpNewMaterial(myMsg, &offset);
	}
	for (int i = 0; i < m_Heder.cameraCount; i++)
	{
		myMsg += offset;
		setUpNewCamera(myMsg, &offset);
	}

    // Load game scene from file
	_scene = Scene::create();
	Camera* cam = Camera::createPerspective(m_Cameras.at(0).FOV, m_Cameras.at(0).aspectRatio, m_Cameras.at(0).Ner, m_Cameras.at(0).Far);
	Node* cameraNode = _scene->addNode("camera");
	getWorldMatrixValus(m_Cameras.at(0).cameraTransform, cameraNode);
	cameraNode->setCamera(cam);
	_scene->setActiveCamera(cam);
	SAFE_RELEASE(cam);

	//_scene = Scene::create();
	//Camera* cam = Camera::createPerspective(45.0f, getAspectRatio(), 1.0, 100.0f);
	//Node* cameraNode = _scene->addNode("camera");
	//cameraNode->setCamera(cam);
	//_scene->setActiveCamera(cam);
	//SAFE_RELEASE(cam);

	//cameraNode->translate(0, 0, 20);
	//cameraNode->rotateX(MATH_DEG_TO_RAD(0.f));
	//cameraNode->rotateX(MATH_DEG_TO_RAD(0.f));

	Node* lightNode = _scene->addNode("light");
	Light* light = Light::createPoint(Vector3(0.5f, 0.5f, 0.5f), 20);
	lightNode->setLight(light);
	SAFE_RELEASE(light);
	lightNode->translate(Vector3(0, 1, 5));

	//Mesh* mesh1 = createCubeMesh(5.0);
	//Mesh* mesh1 = setUpNewMesh(&myMsg);

	for (int i = 0; i < m_Heder.meshCount; i++)
	{
		Material* emtyMat;
		Texture::Sampler* emtySampler;
		mats.push_back(emtyMat);
		samplers.push_back(emtySampler);
		setUpModel(models.at(i), mats.at(i), samplers.at(i), m_Meshes.at(i));
		addNewModel(models.at(i), m_Meshes.at(i));
	}

}

void MayaViewer::finalize()
{
    SAFE_RELEASE(_scene);
}

void MayaViewer::update(float elapsedTime)
{
	static float totalTime = 0;
	totalTime += elapsedTime;
	float step = 360.0 / float(gModelCount);
	char name[10] = {};

	char* myMsg = 0;
	size_t myMsgSize = 0;
	int offset = 0;
	if (myComLib.recv(myMsg, myMsgSize))
	{
		m_SendHeder = *(M_SendHeder*)myMsg;
		m_Heder.meshCount += m_SendHeder.newMesh;
		gModelCount = m_Heder.meshCount;
		m_Heder.materialCount += m_SendHeder.newMaterial;
 		m_Heder.cameraCount += m_SendHeder.newCamera;
		myMsg += sizeof(M_SendHeder);
		std::vector<Model*> models;
		std::vector<Material*> mats;
		std::vector<Texture::Sampler*> samplers;
		
		for (int i = 0; i < m_SendHeder.newMesh; i++)
		{
			myMsg += offset;
			models.push_back(Model::create(setUpNewMesh(myMsg, &offset)));
		}
		for (int i = 0; i < m_SendHeder.newMaterial; i++)
		{
			myMsg += offset;
			setUpNewMaterial(myMsg, &offset);
		}
		for (int i = 0; i < m_SendHeder.newCamera; i++)
		{
			myMsg += offset;
			setUpNewCamera(myMsg, &offset);
		}

		for (int i = 0; i < m_SendHeder.newMesh; i++)
		{
			Material* emtyMat;
			Texture::Sampler* emtySampler;
			mats.push_back(emtyMat);
			samplers.push_back(emtySampler);
			setUpModel(models.at(i), mats.at(i), samplers.at(i), m_Meshes.at((m_Meshes.size()- m_SendHeder.newMesh) + i));
			addNewModel(models.at(i), m_Meshes.at((m_Meshes.size() - m_SendHeder.newMesh) + i));
		}
		if (m_SendHeder.newMesh > 0)
		{
			models.clear();
			mats.clear();
			samplers.clear();
		}

		//new name
		for (int i = 0; i < m_SendHeder.nameChange; i++)
		{
			myMsg += offset;
			M_NewName tempNewName = (*(M_NewName*)myMsg);
			nameChange(myMsg, &offset);
			//updateCamera(tempCamera.id, myMsg, &offset);
		}

		//meshChange
		for (int i = 0; i < m_SendHeder.meshChange; i++)
		{
			myMsg += offset;
			M_Mesh tempMesh = (*(M_Mesh*)myMsg);
			for (int j = 0; j < m_Meshes.size(); j++)
			{
				//if (tempMesh.name == m_Meshes.at(j).name)
				if (std::string(tempMesh.name) == std::string(m_Meshes.at(j).name))
					updateMesh(j, myMsg, &offset);
			}
		}

		for (int i = 0; i < m_SendHeder.materialChange; i++)
		{
			myMsg += offset;
			M_Material tempMaterial = (*(M_Material*)myMsg);
			for (int j = 0; j < m_Materials.size(); j++)
			{
				if (std::string(tempMaterial.name) == std::string(m_Materials.at(j).name))
					updateMaterial(j, myMsg, &offset);
			}
		}

		for (int i = 0; i < m_SendHeder.cameraChange; i++)
		{
			myMsg += offset;
			M_Camera tempCamera = (*(M_Camera*)myMsg);
			updateCamera(tempCamera.id, myMsg, &offset);
		}
	}

	for (int i = 0; i < gModelCount; i++)
	{
		sprintf(name, "cube%d", i);
		Node* node = _scene->findNode(m_Meshes.at(0).name);
		//node->setTranslation(20,0,0);
		//node->setScale(1, 1, 1);
		//const Matrix m = Matrix::identity();
		//Matrix myM;
		//myM.set(Matrix::identity());
		//node->getInverseTransposeWorldMatrix().set(myM);
		
		//if (node) {
		//	node->setScale(0.3f);
		//	node->setTranslation(
		//		cosf(MATH_DEG_TO_RAD(((int)totalTime / 10) % 360 + i * step))*5.0, 
		//		sinf(MATH_DEG_TO_RAD(((int)totalTime / 10) % 360 + i * step))*5.0,
		//		0.0);
		//}
		//if (i%2)
		//	node->rotateX(elapsedTime / 1000.f);
	}	

	Node* camnode = _scene->getActiveCamera()->getNode();
	if (gKeys[Keyboard::KEY_W])
		camnode->translateForward(0.5);
	if (gKeys[Keyboard::KEY_S])
		camnode->translateForward(-0.5);
	if (gKeys[Keyboard::KEY_A])
		camnode->translateLeft(0.5);
	if (gKeys[Keyboard::KEY_D])
		camnode->translateLeft(-0.5);

	if (gMousePressed) {
		camnode->rotate(camnode->getRightVectorWorld(), MATH_DEG_TO_RAD(gDeltaY / 10.0));
		camnode->rotate(camnode->getUpVectorWorld(), MATH_DEG_TO_RAD(gDeltaX / 5.0));
	}
}

void MayaViewer::render(float elapsedTime)
{
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4(0.1f,0.0f,0.0f,1.0f), 1.0f, 0);

    // Visit all the nodes in the scene for drawing
    _scene->visit(this, &MayaViewer::drawScene);
}

bool MayaViewer::drawScene(Node* node)
{
    // If the node visited contains a drawable object, draw it
    Drawable* drawable = node->getDrawable(); 
    if (drawable)
        drawable->draw(_wireframe);

    return true;
}

void MayaViewer::keyEvent(Keyboard::KeyEvent evt, int key)
{
    if (evt == Keyboard::KEY_PRESS)
    {
		gKeys[key] = true;
        switch (key)
        {
        case Keyboard::KEY_ESCAPE:
            exit();
            break;
		};
    }
	else if (evt == Keyboard::KEY_RELEASE)
	{
		gKeys[key] = false;
	}
}

bool MayaViewer::mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta)
{
	static int lastX = 0;
	static int lastY = 0;
	gDeltaX = lastX - x;
	gDeltaY = lastY - y;
	lastX = x;
	lastY = y;
	gMousePressed =
		(evt == Mouse::MouseEvent::MOUSE_PRESS_LEFT_BUTTON) ? true :
		(evt == Mouse::MouseEvent::MOUSE_RELEASE_LEFT_BUTTON) ? false : gMousePressed;

	return true;
}

void MayaViewer::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
    switch (evt)
    {
    case Touch::TOUCH_PRESS:
        _wireframe = !_wireframe;
        break;
    case Touch::TOUCH_RELEASE:
        break;
    case Touch::TOUCH_MOVE:
        break;
    };
}

Mesh* MayaViewer::setUpNewMesh(char* ptr, int* offset)
{
	// make a set upp new mesh later

	m_Meshes.push_back(*(M_Mesh*)ptr);
	ptr += sizeof(M_Mesh);
	*offset = sizeof(M_Mesh);
	std::vector<M_Vertice> meshVtx;
	//M_Vertice* meshVtx = new M_Vertice[m_Meshes.back().verticeCount];
	float* verteces = new float[m_Meshes.back().verticeCount * 8];
	short* meshIndexVtx = new short[m_Meshes.back().indexCount];
	std::vector<M_VertexIndex> temp;
	for (int i = 0; i < m_Meshes.back().verticeCount; i++)
	{
		//meshVtx[i] = *(M_Vertice*)ptr;
		meshVtx.push_back(*(M_Vertice*)ptr);
		verteces[i * 8] = meshVtx.back().pos[0];
		verteces[i * 8 + 1] = meshVtx.back().pos[1];
		verteces[i * 8 + 2] = meshVtx.back().pos[2];
		verteces[i * 8 + 3] = meshVtx.back().normal[0];
		verteces[i * 8 + 4] = meshVtx.back().normal[1];
		verteces[i * 8 + 5] = meshVtx.back().normal[2];
		verteces[i * 8 + 6] = meshVtx.back().uv[0];
		verteces[i * 8 + 7] = meshVtx.back().uv[1];
		float u = verteces[i * 8 + 6];
		float v = verteces[i * 8 + 7];
		ptr += sizeof(M_Vertice);
		*offset += sizeof(M_Vertice);
	}
	for (int i = 0; i < m_Meshes.back().indexCount; i++)
	{
		//meshIndexVtx[i] = *(int*)ptr;
		temp.push_back(*(M_VertexIndex*)ptr);
		meshIndexVtx[i] = temp.back().index;
		ptr += sizeof(M_VertexIndex);
		*offset += sizeof(M_VertexIndex);
	}
	////unsigned int vertexCount = 24;
	////unsigned int indexCount = 36;
	//VertexFormat::Element elements[] =
	//{
	//	VertexFormat::Element(VertexFormat::POSITION, 3),
	//	VertexFormat::Element(VertexFormat::NORMAL, 3),
	//	VertexFormat::Element(VertexFormat::TEXCOORD0, 2)
	//};
	//Mesh* mesh = Mesh::createMesh(VertexFormat(elements, 3), m_Meshes.back().verticeCount, false);
	//if (mesh == NULL)
	//{
	//	GP_ERROR("Failed to create mesh.");
	//	return NULL;
	//}
	//mesh->setVertexData(verteces, 0, m_Meshes.back().verticeCount);
	//MeshPart* meshPart = mesh->addPart(Mesh::TRIANGLES, Mesh::INDEX16, m_Meshes.back().indexCount, false);
	//meshPart->setIndexData(meshIndexVtx, 0, m_Meshes.back().indexCount);

	VertexFormat::Element elements[] =
	{
		VertexFormat::Element(VertexFormat::POSITION, 3),
		VertexFormat::Element(VertexFormat::NORMAL, 3),
		VertexFormat::Element(VertexFormat::TEXCOORD0, 2)
	};
	Mesh* mesh = Mesh::createMesh(VertexFormat(elements, 3), m_Meshes.back().verticeCount, false);
	if (mesh == NULL)
	{
		GP_ERROR("Failed to create mesh.");
		return NULL;
	}
	mesh->setVertexData(verteces, 0, m_Meshes.back().verticeCount);
	MeshPart* meshPart = mesh->addPart(Mesh::TRIANGLES, Mesh::INDEX16, m_Meshes.back().indexCount, false);
	meshPart->setIndexData(meshIndexVtx, 0, m_Meshes.back().indexCount);

	//delete[] meshVtx;
	delete[] meshIndexVtx;
	return mesh;
}

void MayaViewer::setUpNewMaterial(char * ptr, int * offset)
{
	m_Materials.push_back(*(M_Material*)ptr);
	ptr += sizeof(M_Material);
	*offset = sizeof(M_Material);
}

void MayaViewer::setUpNewCamera(char * ptr, int * offset)
{
	m_Cameras.push_back(*(M_Camera*)ptr);
	ptr += sizeof(M_Camera);
	*offset = sizeof(M_Camera);
}

void MayaViewer::updateCamera(int id, char * ptr, int * offset)
{
	m_Cameras.at(id) = (*(M_Camera*)ptr);
	ptr += sizeof(M_Camera);
	*offset = sizeof(M_Camera);
	Node* node = _scene->findNode("camera");
	Camera* cam;
	if(m_Cameras.at(id).isOrtho)
		cam = Camera::createOrthographic(m_Cameras.at(id).oWhith + (m_Cameras.at(id).oWhith*0.75f), m_Cameras.at(id).oWhith, m_Cameras.at(id).aspectRatio, m_Cameras.at(id).Ner, m_Cameras.at(id).Far);
	else
		cam = Camera::createPerspective(m_Cameras.at(0).FOV, m_Cameras.at(0).aspectRatio, m_Cameras.at(0).Ner, m_Cameras.at(0).Far);
	node->setCamera(cam);
	_scene->setActiveCamera(cam);
	SAFE_RELEASE(cam);
	getWorldMatrixValus(m_Cameras.at(id).cameraTransform, node);
}

void MayaViewer::updateMesh(int id, char * ptr, int * offset)
{
	m_Meshes.at(id) = (*(M_Mesh*)ptr);
	ptr += sizeof(M_Mesh);
	*offset = sizeof(M_Mesh);
	Node* node = _scene->findNode(m_Meshes.at(id).name);
	if(node != NULL)
		getWorldMatrixValus(m_Meshes.at(id).meshTransform, node);

	if (m_Meshes.at(id).verticeCount != 0 || m_Meshes.at(id).materialID  == -1 )
	{
		std::vector<M_Vertice> meshVtx;
		//M_Vertice* meshVtx = new M_Vertice[m_Meshes.at(id).verticeCount];
		float* verteces = new float[m_Meshes.at(id).verticeCount * 8];
		short* meshIndexVtx = new short[m_Meshes.at(id).indexCount];
		std::vector<M_VertexIndex> temp;
		for (int i = 0; i < m_Meshes.at(id).verticeCount; i++)
		{
			//meshVtx[i] = *(M_Vertice*)ptr;
			meshVtx.push_back(*(M_Vertice*)ptr);
			verteces[i * 8] = meshVtx.back().pos[0];
			verteces[i * 8 + 1] = meshVtx.back().pos[1];
			verteces[i * 8 + 2] = meshVtx.back().pos[2];
			verteces[i * 8 + 3] = meshVtx.back().normal[0];
			verteces[i * 8 + 4] = meshVtx.back().normal[1];
			verteces[i * 8 + 5] = meshVtx.back().normal[2];
			verteces[i * 8 + 6] = meshVtx.back().uv[0];
			verteces[i * 8 + 7] = meshVtx.back().uv[1];
			ptr += sizeof(M_Vertice);
			*offset += sizeof(M_Vertice);
		}
		for (int i = 0; i < m_Meshes.at(id).indexCount; i++)
		{
			//meshIndexVtx[i] = *(int*)ptr;
			temp.push_back(*(M_VertexIndex*)ptr);
			meshIndexVtx[i] = temp.back().index;
			ptr += sizeof(M_VertexIndex);
			*offset += sizeof(M_VertexIndex);
		}

		VertexFormat::Element elements[] =
		{
			VertexFormat::Element(VertexFormat::POSITION, 3),
			VertexFormat::Element(VertexFormat::NORMAL, 3),
			VertexFormat::Element(VertexFormat::TEXCOORD0, 2)
		};
		Mesh* mesh = Mesh::createMesh(VertexFormat(elements, 3), m_Meshes.at(id).verticeCount, false);
		//if (mesh == NULL)
		//{
		//	GP_ERROR("Failed to create mesh.");
		//	return NULL;
		//}
		mesh->setVertexData(verteces, 0, m_Meshes.at(id).verticeCount);
		MeshPart* meshPart = mesh->addPart(Mesh::TRIANGLES, Mesh::INDEX16, m_Meshes.at(id).indexCount, false);
		meshPart->setIndexData(meshIndexVtx, 0, m_Meshes.at(id).indexCount);
		
		Model* model = Model::create(mesh);
		Material* myMaterial = NULL;
		Texture::Sampler* mySampler = NULL;
		if (m_Meshes.at(id).materialID != -1)
		{
			setUpModel(model, myMaterial, mySampler, m_Meshes.at(id));
			changeModel(model, m_Meshes.at(id));
		}
		else
		{
			char nodeName[50] = {};
			sprintf(nodeName, m_Meshes.at(id).name);
			Node* removeNode = _scene->findNode(nodeName);
			if(removeNode != NULL)
				_scene->removeNode(removeNode);
		}
		////delete[] meshVtx;
		delete[] meshIndexVtx;
		//return mesh;
	}
}

void MayaViewer::updateMaterial(int id, char * ptr, int * offset)
{
	m_Materials.at(id) = *(M_Material*)ptr;
	ptr += sizeof(M_Material);
	*offset = sizeof(M_Material);
	for (int i = 0; i < m_Meshes.size(); i++)
	{
		if (m_Meshes.at(i).materialID == id)
		{
			char nodeName[50] = {};
			sprintf(nodeName, m_Meshes.at(i).name);
			Node* nodeToChange = _scene->findNode(nodeName);
			Model* myModel = (Model*)nodeToChange->getDrawable();
			Material* myMaterial = NULL;
			Texture::Sampler* mySampler = NULL;
			setUpModel(myModel, myMaterial, mySampler, m_Meshes.at(i));
			nodeToChange->setDrawable(myModel);
		}
	}
}

void MayaViewer::nameChange(char * ptr, int * offset)
{
	M_NewName nameInfo = (*(M_NewName*)ptr);
	ptr += sizeof(M_NewName);
	*offset = sizeof(M_NewName);
	for (int i = 0; i < m_Meshes.size(); i++)
	{
		for (int i = 0; i < m_Meshes.size(); i++)
			if (std::string(nameInfo.oldName) == std::string(m_Meshes.at(i).name))
				std::strcpy(m_Meshes.at(i).name, nameInfo.newName);
	}
	for (int i = 0; i < m_Materials.size(); i++)
	{
		for (int i = 0; i < m_Materials.size(); i++)
			if (std::string(nameInfo.oldName) == std::string(m_Materials.at(i).name))
				std::strcpy(m_Materials.at(i).name, nameInfo.newName);
	}
	Node* node = _scene->findNode(nameInfo.oldName);
	if (node != NULL)
	{
		node->setId(nameInfo.newName);
	}
}

void MayaViewer::setUpModel(Model* model, Material* mat, Texture::Sampler* sampler, M_Mesh mesh)
{
	Node* lightNode = _scene->findNode("light");


	if (m_Materials.at(mesh.materialID).diffuseMap[0] != '\0')
	{
		mat = model->setMaterial("res/shaders/textured.vert", "res/shaders/textured.frag", "POINT_LIGHT_COUNT 1");
		mat->setParameterAutoBinding("u_worldViewProjectionMatrix", "WORLD_VIEW_PROJECTION_MATRIX");
		mat->setParameterAutoBinding("u_inverseTransposeWorldViewMatrix", "INVERSE_TRANSPOSE_WORLD_VIEW_MATRIX");
		mat->getParameter("u_ambientColor")->setValue(Vector3(0.1f, 0.1f, 0.2f));
		mat->getParameter("u_pointLightColor[0]")->setValue(lightNode->getLight()->getColor());
		mat->getParameter("u_pointLightPosition[0]")->bindValue(lightNode, &Node::getTranslationWorld);
		mat->getParameter("u_pointLightRangeInverse[0]")->bindValue(lightNode->getLight(), &Light::getRangeInverse);
		//sampler = mat->getParameter("u_diffuseTexture")->setValue("res/png/crate.png", true);
		//bara png!?
		sampler = mat->getParameter("u_diffuseTexture")->setValue(m_Materials.at(mesh.materialID).diffuseMap, true);
		// funkar inte för saker utanför res
		//samplers.push_back(mats.at(i)->getParameter("u_diffuseTexture")->setValue(m_Materials.at(m_Meshes.at(i).materialID).diffuseMap, true));
		sampler->setFilterMode(Texture::LINEAR_MIPMAP_LINEAR, Texture::LINEAR);
		mat->getStateBlock()->setCullFace(true);
		mat->getStateBlock()->setDepthTest(true);
		mat->getStateBlock()->setDepthWrite(true);
	}
	else
	{
		mat = model->setMaterial("res/shaders/colored.vert", "res/shaders/colored.frag", "POINT_LIGHT_COUNT 1");
		mat->setParameterAutoBinding("u_worldViewProjectionMatrix", "WORLD_VIEW_PROJECTION_MATRIX");
		mat->setParameterAutoBinding("u_inverseTransposeWorldViewMatrix", "INVERSE_TRANSPOSE_WORLD_VIEW_MATRIX");
		mat->getParameter("u_ambientColor")->setValue(Vector3(0.1f, 0.1f, 0.2f));
		mat->getParameter("u_pointLightColor[0]")->setValue(lightNode->getLight()->getColor());
		mat->getParameter("u_pointLightPosition[0]")->bindValue(lightNode, &Node::getTranslationWorld);
		mat->getParameter("u_pointLightRangeInverse[0]")->bindValue(lightNode->getLight(), &Light::getRangeInverse);
		//mats.at(i)->getParameter("u_diffuseColor")->setValue(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		mat->getParameter("u_diffuseColor")->setValue(Vector4(m_Materials.at(mesh.materialID).color[0], m_Materials.at(mesh.materialID).color[1], m_Materials.at(mesh.materialID).color[2], 1.0f));
		//mats.at(i)->getParameter("u_ambientColor")->setValue(Vector3(m_Materials.at(m_Meshes.at(i).materialID).color[0], m_Materials.at(m_Meshes.at(i).materialID).color[1], m_Materials.at(m_Meshes.at(i).materialID).color[2]));
		//samplers.push_back(mats.at(i)->getParameter("u_diffuseTexture")->setValue("res/png/crate.png", true));
		//samplers.at(i)->setFilterMode(Texture::LINEAR_MIPMAP_LINEAR, Texture::LINEAR);
		mat->getStateBlock()->setCullFace(true);
		mat->getStateBlock()->setDepthTest(true);
		mat->getStateBlock()->setDepthWrite(true);
	}

	//sprintf(nodeName, "cube%d", i);
	//Node* node = _scene->addNode(nodeName);
}

void MayaViewer::addNewModel(Model* model, M_Mesh mesh)
{
	char nodeName[50] = {};
	sprintf(nodeName, mesh.name);
	Node* node = _scene->addNode(nodeName);
	getWorldMatrixValus(mesh.meshTransform, node);
	node->setDrawable(model);
	SAFE_RELEASE(model);
}

void MayaViewer::changeModel(Model* model, M_Mesh mesh)
{
	char nodeName[50] = {};
	sprintf(nodeName, mesh.name);
	Node* node = _scene->findNode(nodeName);
	_scene->removeNode(node);
	node = _scene->addNode(nodeName);
	getWorldMatrixValus(mesh.meshTransform, node);
	node->setDrawable(model);
	SAFE_RELEASE(model);
}

void MayaViewer::getWorldMatrixValus(M_Trasform curentTransform, Node * curentNode)
{
	Vector3 translate;
	Quaternion rotation;
	Vector3 scale;
	float f[16];
	for (int x = 0; x < 4; x++)
		for (int y = 0; y < 4; y++)
			f[x * 4 + y] = curentTransform.TransformMatrix[y][x];
	Matrix myM(f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], f[12], f[13], f[14], f[15]);
	myM.getTranslation(&translate);
	myM.getRotation(&rotation);
	myM.getScale(&scale);
	curentNode->setTranslation(translate);
	curentNode->setScale(scale);
	curentNode->setRotation(rotation);
}

Mesh* MayaViewer::createCubeMesh(float size)
{
	float a = 0.5f;
	float vertices[] =
	{
		-a*2, -a*2,  a*2,    0.0,  0.0,  1.0,   0.0, 0.0,
		a*2, -a*2,  a*2,    0.0,  0.0,  1.0,   1.0, 0.0,
		a*2,  a*2,  a*2,    0.0,  0.0,  1.0,   1.0, 1.0,
		-a*2,  a*2,  a*2,    0.0,  0.0,  1.0,   0.0, 1.0,
		-a,  a,  a,    0.0,  1.0,  0.0,   0.0, 0.0,
		a,  a,  a,    0.0,  1.0,  0.0,   1.0, 0.0,
		-a,  a, -a,    0.0,  1.0,  0.0,   0.0, 1.0,
		a,  a, -a,    0.0,  1.0,  0.0,   1.0, 1.0,
		-a,  a, -a,    0.0,  0.0, -1.0,   0.0, 0.0,
		a,  a, -a,    0.0,  0.0, -1.0,   1.0, 0.0,
		-a, -a, -a,    0.0,  0.0, -1.0,   0.0, 1.0,
		a, -a, -a,    0.0,  0.0, -1.0,   1.0, 1.0,
		-a, -a, -a,    0.0, -1.0,  0.0,   0.0, 0.0,
		a, -a, -a,    0.0, -1.0,  0.0,   1.0, 0.0,
		-a, -a,  a,    0.0, -1.0,  0.0,   0.0, 1.0,
		a, -a,  a,    0.0, -1.0,  0.0,   1.0, 1.0,
		a, -a,  a,    1.0,  0.0,  0.0,   0.0, 0.0,
		a, -a, -a,    1.0,  0.0,  0.0,   1.0, 0.0,
		a,  a,  a,    1.0,  0.0,  0.0,   0.0, 1.0,
		a,  a, -a,    1.0,  0.0,  0.0,   1.0, 1.0,
		-a, -a, -a,   -1.0,  0.0,  0.0,   0.0, 0.0,
		-a, -a,  a,   -1.0,  0.0,  0.0,   1.0, 0.0,
		-a,  a, -a,   -1.0,  0.0,  0.0,   0.0, 1.0,
		-a,  a,  a,   -1.0,  0.0,  0.0,   1.0, 1.0
	};
	short indices[] =
	{
		0, 1, 2, 0, 2, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 18, 17, 19, 20, 21, 22, 22, 21, 23
	};
	unsigned int vertexCount = 24;
	unsigned int indexCount = 36;
	VertexFormat::Element elements[] =
	{
		VertexFormat::Element(VertexFormat::POSITION, 3),
		VertexFormat::Element(VertexFormat::NORMAL, 3),
		VertexFormat::Element(VertexFormat::TEXCOORD0, 2)
	};
	Mesh* mesh = Mesh::createMesh(VertexFormat(elements, 3), vertexCount, false);
	if (mesh == NULL)
	{
		GP_ERROR("Failed to create mesh.");
		return NULL;
	}
	mesh->setVertexData(vertices, 0, vertexCount);
	MeshPart* meshPart = mesh->addPart(Mesh::TRIANGLES, Mesh::INDEX16, indexCount, false);
	meshPart->setIndexData(indices, 0, indexCount);
	return mesh;
}
