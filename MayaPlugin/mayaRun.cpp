#include "maya_includes.h"
#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include "ComLib.h"

using namespace std;
MCallbackIdArray callbackIdArray;
MObject m_node;
MStatus status = MS::kSuccess;
bool initBool = false;

enum NODE_TYPE { TRANSFORM, MESH };
MTimer gTimer;
double lastTime = 0;
double changeTimer = 0;

// keep track of created meshes to maintain them
queue<MObject> newMeshes;
vector<MObject> allMeshes;
vector<MObject> allCameras;
vector<int> nrOfVtx;
int meshIdToUpdate = -1;
float timeFromStart = 0;


//filformat
struct Heder
{
	int meshCount = 0;
	int materialCount = 0;
	int cameraCount = 0;
	int lightCount = 0;
};

//the number of changes
struct sendHeder
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

struct newName
{
	char oldName[50];
	char newName[50];
};

struct Trasform
{
	float TransformMatrix[4][4];
};

struct Mesh
{
	char name[50];
	int verticeCount = 0;
	int indexCount = 0;
	int materialID = -1;
	Trasform meshTransform;
};

struct VertexIndex
{
	int index;
};

struct Vertice
{
	float pos[3];
	float normal[3];
	float uv[2];
	/*float tangent[3];
	float bitangent[3];*/
};

struct MeshVertexIndex
{
	vector<VertexIndex> meshVtxIndex;
};

struct MeshVertices
{
	vector<Vertice> meshVtxs;
};

struct Material
{
	char name[50];
	float color[4];
	//bool mapsDSN[3];
	char diffuseMap[200] = "";
	//char specularMap[100];
	//char normalMap[100];
};

struct Camera
{
	int id;
	bool isOrtho;
	//float pos[3];
	//float upVec[3];
	//float direction[3];
	float aspectRatio;
	float FOV, Ner, Far;
	float oWhith;
	Trasform cameraTransform;
};

struct Light
{
	char type[50];
	//float pos[3];
	int intensety;
	float color[4];
	Trasform lightTransform;
};

struct ChangeMesh
{
	int id;
	bool vtx = false;
};

Heder m_Heder;
sendHeder m_SendHeder;
vector<Mesh> m_meshes;
vector<MeshVertices> m_meshVtx;
vector<MeshVertexIndex> m_meshVtxIndex;
vector<Material> m_meshMaterial;
vector<ChangeMesh> m_changeMeshID;
vector<int> m_changeCameraID;
vector<int> m_changeMaterialID;

vector<Camera> m_Cameras;
vector<Camera> m_CamerasCurentTransforms;

vector <newName> m_nameShanges;

int onlySomeTimes = 0;

ComLib myComLib = ComLib("MayaKey", 100 *1 << 20, ComLib::TYPE::PRODUCER);

void nodeAttrebuteChange(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void*);
void dirtyNodePlug(MObject &node, MPlug &plug, void *clientData);


void sendDataToComLib()
{
	//cout << "check" << endl;
	//order
	//int newMesh = 0;
	//int newMaterial = 0;
	//int newCamera = 0;
	//int newLight = 0;
	//int nameChange = 0;
	//int meshChange = 0;
	//int materialChange = 0;
	//int cameraChange = 0;
	//int lightChange = 0;
	size_t sendSize = sizeof(m_SendHeder);
	for (int i = m_Heder.meshCount - m_SendHeder.newMesh; i < m_Heder.meshCount; i++)
	{
		sendSize += sizeof(Mesh);
		sendSize += sizeof(Vertice) * m_meshes.at(i).verticeCount;
		sendSize += sizeof(VertexIndex) * m_meshes.at(i).indexCount;
	}
	for (int i = m_Heder.materialCount - m_SendHeder.newMaterial; i < m_Heder.materialCount; i++)
	{
		sendSize += sizeof(Material);
	}
	for (int i = m_Heder.cameraCount - m_SendHeder.newCamera; i < m_Heder.cameraCount; i++)
	{
		sendSize += sizeof(Camera);
	}
	//
	for (int i = 0; i < m_SendHeder.nameChange; i++)
	{
		sendSize += sizeof(newName);
	}
	for (int i = 0; i < m_SendHeder.meshChange; i++)
	{
		sendSize += sizeof(Mesh);
		sendSize += sizeof(Vertice) * m_meshes.at(m_changeMeshID.at(i).id).verticeCount;
		sendSize += sizeof(VertexIndex) * m_meshes.at(m_changeMeshID.at(i).id).indexCount;
	}
	for (int i = 0; i < m_SendHeder.materialChange; i++)
	{
		sendSize += sizeof(Material);
	}
	for (int i = 0; i < m_SendHeder.cameraChange; i++)
	{
		sendSize += sizeof(Camera);
	}
	//
	if (myComLib.canFit(sendSize) && sendSize > sizeof(m_SendHeder))
	{
		//cout << "the size to send is: " << sendSize << endl;
		//cout << "have sent somthing " << endl;
		char *infoToSend;
		infoToSend = new char[sendSize];
		//void* pointToData = infoToSend;
		int testt[10];
		void* basePtr = infoToSend;
		void* myPtr = infoToSend;

		memcpy(myPtr, &m_SendHeder, sizeof(m_SendHeder));
		myPtr = (char*)myPtr + sizeof(m_SendHeder);

		for (int i = m_Heder.meshCount - m_SendHeder.newMesh; i < m_Heder.meshCount; i++)
		{
			memcpy(myPtr, &m_meshes.at(i), sizeof(Mesh));
			myPtr = (char*)myPtr + sizeof(m_meshes.at(i));
			for (int j = 0; j < m_meshes.at(i).verticeCount; j++)
			{
				memcpy(myPtr, &m_meshVtx.at(i).meshVtxs.at(j), sizeof(Vertice));
				myPtr = (char*)myPtr + sizeof(Vertice);
			}
			for (int j = 0; j < m_meshes.at(i).indexCount; j++)
			{
				memcpy(myPtr, &m_meshVtxIndex.at(i).meshVtxIndex.at(j), sizeof(VertexIndex));
				myPtr = (char*)myPtr + sizeof(VertexIndex);
			}
			//cout << "have sent " << sendSize << endl;
		}
		m_SendHeder.newMesh = 0;

		for (int i = m_Heder.materialCount - m_SendHeder.newMaterial; i < m_Heder.materialCount; i++)
		{
			//cout << "myColor: " << m_meshMaterial.at(i).color[0] << " " << m_meshMaterial.at(i).color[1] << " " << m_meshMaterial.at(i).color[2] << " " << endl;
			memcpy(myPtr, &m_meshMaterial.at(i), sizeof(Material));
			myPtr = (char*)myPtr + sizeof(Material);
		}
		m_SendHeder.newMaterial = 0;

		for (int i = m_Heder.cameraCount - m_SendHeder.newCamera; i < m_Heder.cameraCount; i++)
		{
			memcpy(myPtr, &m_Cameras.at(i), sizeof(Camera));
			myPtr = (char*)myPtr + sizeof(Camera);
		}
		m_SendHeder.newCamera = 0;
		
		for (int i = 0; i < m_SendHeder.nameChange; i++)
		{
			memcpy(myPtr, &m_nameShanges.at(i), sizeof(newName));
			myPtr = (char*)myPtr + sizeof(newName);
		}
		m_nameShanges.clear();
		m_SendHeder.nameChange = 0;

		for (int i = 0; i < m_SendHeder.meshChange; i++)
		{
			Mesh sendMesh = m_meshes.at(m_changeMeshID.at(i).id);
			if (!m_changeMeshID.at(i).vtx)
			{
				sendMesh.indexCount = 0;
				sendMesh.verticeCount = 0;
			}
			memcpy(myPtr, &sendMesh, sizeof(Mesh));
			myPtr = (char*)myPtr + sizeof(Mesh);
			for (int j = 0; j < sendMesh.verticeCount; j++)
			{
				memcpy(myPtr, &m_meshVtx.at(m_changeMeshID.at(i).id).meshVtxs.at(j), sizeof(Vertice));
				myPtr = (char*)myPtr + sizeof(Vertice);
			}
			for (int j = 0; j < sendMesh.indexCount; j++)
			{
				memcpy(myPtr, &m_meshVtxIndex.at(m_changeMeshID.at(i).id).meshVtxIndex.at(j), sizeof(VertexIndex));
				myPtr = (char*)myPtr + sizeof(VertexIndex);
			}
			if (m_meshes.at(m_changeMeshID.at(i).id).materialID == -1)
			{
				char newName[50] = "removed";
				for (int s = 0; s < 50; s++)
				{
					m_meshes.at(m_changeMeshID.at(i).id).name[s] = newName[s];
				}
			}
		}
		m_changeMeshID.clear();
		m_SendHeder.meshChange = 0;
		for (int i = 0; i < m_SendHeder.materialChange; i++)
		{
			//sendSize += sizeof(Material);
			memcpy(myPtr, &m_meshMaterial.at(m_changeMaterialID.at(i)), sizeof(Material));
			myPtr = (char*)myPtr + sizeof(Material);
		}
		m_changeMaterialID.clear();
		m_SendHeder.materialChange = 0;
		for (int i = 0; i < m_SendHeder.cameraChange; i++)
		{
			//sendSize += sizeof(Camera);
			memcpy(myPtr, &m_Cameras.at(m_changeCameraID.at(i)), sizeof(Camera));
			myPtr = (char*)myPtr + sizeof(Camera);
		}
		m_changeCameraID.clear();
		m_SendHeder.cameraChange = 0;
		
		myComLib.send(basePtr, sendSize);
		myPtr = NULL;
		basePtr = NULL;
		delete basePtr;
		delete myPtr;
		delete[] infoToSend;
	}
}

void sendToComLib(float elapsedTime, float lastTime, void*)
{
	sendDataToComLib();
	for (size_t i = 0; i < m_Cameras.size(); i++)
		for (int x = 0; x < 4; x++)
			for (int y = 0; y < 4; y++)
				m_CamerasCurentTransforms.at(i).cameraTransform.TransformMatrix[x][y] = m_Cameras.at(i).cameraTransform.TransformMatrix[x][y];
}


/*
void nodeAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	// cout << plug.name() << ": " << plug.partialName() << endl;
}
*/

/*
 * how Maya calls this method when a node is added.
 * new POLY mesh: kPolyXXX, kTransform, kMesh
 * new MATERIAL : kBlinn, kShadingEngine, kMaterialInfo
 * new LIGHT    : kTransform, [kPointLight, kDirLight, kAmbientLight]
 * new JOINT    : kJoint
 */
void nodeNameChange(MObject &node, const MString &str, void*){
	cout << "New name:" << endl;
	//cout << str.asChar() << endl;
	//(node.hasFn(MFn::kDagNode))
	if (node.hasFn(MFn::MFn::kLambert))
	{
		MFnLambertShader myDagNode = node;
		cout << "Material" << endl;
		cout << "Old name: " << str.asChar() << endl;
		cout << "New name: " << myDagNode.name() << endl;
		for (int i = 0; i < m_meshMaterial.size(); i++)
		{
			if (string(str.asChar()) == string(m_meshMaterial.at(i).name))
			{
				strcpy(m_meshMaterial.at(i).name, myDagNode.name().asChar());
				cout << "transferd name " << m_meshMaterial.at(i).name << endl;

				newName myNewName;
				strcpy(myNewName.oldName, str.asChar());
				strcpy(myNewName.newName, myDagNode.name().asChar());
				m_nameShanges.push_back(myNewName);
				m_SendHeder.nameChange++;
			}
		}
	}
	if (node.hasFn(MFn::kMesh))
	{
		MFnDagNode myDagNode = node;
		cout << "mesh"<< endl;
		cout << "Old name: " << str.asChar() << endl;
		cout << "New name: " << myDagNode.name() << endl;
		for (int i = 0; i < m_meshes.size(); i++)
		{
			if (string(str.asChar()) == string(m_meshes.at(i).name))
			{
				strcpy(m_meshes.at(i).name, myDagNode.name().asChar());
				cout << "transferd name " << m_meshes.at(i).name << endl;

				newName myNewName;
				strcpy(myNewName.oldName, str.asChar());
				strcpy(myNewName.newName, myDagNode.name().asChar());
				m_nameShanges.push_back(myNewName);
				m_SendHeder.nameChange++;
			}
		}
	}
	if (node.hasFn(MFn::kTransform))
	{
		MFnDagNode myDagNode = node;
		cout << "transform" << endl;
		cout << "Old name: " << str.asChar() << endl;
		cout << "New name: " << myDagNode.name() << endl;
	}
	//MGlobal::displayInfo("......Name.....");
}

void nodeAttrebuteAddedOrRemoved(MNodeMessage::AttributeMessage msg, MPlug &plug, void*) {
	//MGlobal::displayInfo("New attribut?");
	cout << "New attribut?............................." << endl;
}

void timerCallback(float elapsedTime, float lastTime, void*)
{
	//cout << "its time" << endl;
	cout << "elapsed time " << elapsedTime << endl;
	//timeFromStart += elapsedTime;
	//cout << "elapsed time from start " << timeFromStart << endl;
	cout << ":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::" << endl;
	cout << "number of meshes: " << m_Heder.meshCount << endl;
	for (int i = 0; i < m_Heder.meshCount; i++)
	{
		cout << "mesh: " << i << endl;
		cout << "---------------------" << endl;
		cout << "mesh transform: " << endl;// m_meshes.at(i).meshTransform.TransformMatrix[0][0] << endl;
		for (int j = 0; j < 4; j++)
		{
			cout << "(";
			for (int k = 0; k < 4; k++)
			{
				cout << m_meshes.at(i).meshTransform.TransformMatrix[j][k];
				if (k < 3)
					cout << ",	";
			}
			cout << ") " << endl;
		}
		cout << endl << "number of vertices: " << m_meshes.at(i).verticeCount << endl;
		cout << "number of total vertices: " << m_meshes.at(i).indexCount << endl;
		cout << "---------------------" << endl;
		for (int j = 0; j < m_meshes.at(i).verticeCount; j++)
		{
			cout << "mesh: " << i << "		vertice: " << j << endl;
			cout << "Pos: " << m_meshVtx.at(i).meshVtxs.at(j).pos[0] << "   " << m_meshVtx.at(i).meshVtxs.at(j).pos[1] << "   "
				<< m_meshVtx.at(i).meshVtxs.at(j).pos[2] << endl;
			cout << "Normal: " << m_meshVtx.at(i).meshVtxs.at(j).normal[0] << "   " << m_meshVtx.at(i).meshVtxs.at(j).normal[1] << "   "
				<< m_meshVtx.at(i).meshVtxs.at(j).normal[2] << endl;
			cout << "UV: " << m_meshVtx.at(i).meshVtxs.at(j).uv[0] << "   " << m_meshVtx.at(i).meshVtxs.at(j).uv[1]  << endl;
		}
		cout << "---------------------" << endl;
		for (int j = 0; j < m_meshes.at(i).indexCount; j++)
		{
			cout << m_meshVtxIndex.at(i).meshVtxIndex.at(j).index << ", ";
		}
	}
	cout << endl << ":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::" << endl << endl;

}

Material getMaterial(MFnMesh &mesh)
{
	unsigned int instensNr = 0;
	MObjectArray shaders;
	MIntArray shaderIndices;
	mesh.getConnectedShaders(instensNr, shaders, shaderIndices);

	Material tempMaterial;
	//cout << "shader data: " << endl;
	//cout << "nr of shaders? " << shaders.length() << endl;
	////funkar inte om ett objekt har mer än 3 materials
	MObject shaderObject[3];
	shaders.get(shaderObject);
	//finkar flr lambet, blinn, phong (alla?)
	MItDependencyGraph shaderIt(shaderObject[0], MFn::kLambert, MItDependencyGraph::kUpstream, MItDependencyGraph::kDepthFirst, MItDependencyGraph::kNodeLevel, &status);
	//funkar bara för blinn
	//MItDependencyGraph shaderIt(shaderObject[0], MFn::kBlinn, MItDependencyGraph::kUpstream, MItDependencyGraph::kDepthFirst, MItDependencyGraph::kNodeLevel, &status);
	MObject myShader = shaderIt.thisNode();
	MFnLambertShader testLambert = myShader;
	//cout << "shader name? " << testLambert.name() << endl;
	for (int i = 0; i <= testLambert.name().length(); i++)
	{
		tempMaterial.name[i] = testLambert.name().asChar()[i];
	}

	//cout << "shader color: " << testLambert.color() << endl;
	tempMaterial.color[0] = testLambert.color().r;
	tempMaterial.color[1] = testLambert.color().g;
	tempMaterial.color[2] = testLambert.color().b;


	//get the diffuse/color textur
	MFnDependencyNode fn(myShader);
	MPlug colorPlug = fn.findPlug("color");
	MPlugArray myColorPlugs;
	colorPlug.connectedTo(myColorPlugs, true, false);
	for (int i = 0; i < myColorPlugs.length(); i++)
	{
		//cout << "a textur?" << endl;
		if (myColorPlugs[i].node().hasFn(MFn::kFileTexture))
		{
			//cout << "I found a textur!" << endl << "it is a diffuse map" << endl;
			MFnDependencyNode textur(myColorPlugs[i].node());
			MPlug texturNamePlug = textur.findPlug("fileTextureName");
			MString myFilePath;
			texturNamePlug.getValue(myFilePath);
			//cout << myFilePath << endl;
			for (int j = 0; j < myFilePath.length(); j++)
				tempMaterial.diffuseMap[j] = myFilePath.asChar()[j];
			//cout << tempMaterial.diffuseMap << endl;
		}
	}

	//get the normal txture
	//MFnDependencyNode fn(myShader);
	MPlug bumpPlug = fn.findPlug("normalCamera");
	MPlugArray myBumpPlugs;
	bumpPlug.connectedTo(myBumpPlugs, true, false);
	for (int i = 0; i < myBumpPlugs.length(); i++)
	{
		//cout << "a conection" << endl;
		MFnDependencyNode bumpTexturNode(myBumpPlugs[i].node());
		//cout << "node name " << bumpTexturNode.name() << endl;
		MPlug bumpTxPlug = bumpTexturNode.findPlug("bumpValue");
		MPlugArray myBumpTxs;
		bumpTxPlug.connectedTo(myBumpTxs, true, false);
		for (int j = 0; j < myBumpTxs.length(); j++)
		{
			//cout << "a textur?" << endl;
			if (myBumpTxs[j].node().hasFn(MFn::kFileTexture))
			{
				//cout << "I found a textur!" << endl << "it is a normal map" << endl;
				MFnDependencyNode textur(myBumpTxs[j].node());
				MPlug texturNamePlug = textur.findPlug("fileTextureName");
				MString myFilePath;
				texturNamePlug.getValue(myFilePath);
				//cout << myFilePath << endl;
			}
		}
	}

	//get the speculer textur?
	//-----
	// add here
	//----
	return tempMaterial;
}

Vertice calcVtx(MFnMesh &mesh ,int vtxID, int faceID, float u, float v)
{
	Vertice tVtx;
	MPoint pointPos;
	float vtxPos[4];
	MVector normal;
	//tVtx.id = vtxID;
	const MString *uvString = NULL;
	float ux, vx;
	mesh.getPoint(vtxID, pointPos, MSpace::kObject);
	mesh.getFaceVertexNormal(faceID, vtxID, normal, MSpace::kObject);
	//cout << "i och j: " << faceID << vtxID << endl;
	mesh.getPolygonUV(faceID, vtxID, ux, vx, uvString);
	pointPos.get(vtxPos);

	//cout << "pos: " << endl;
	for (int k = 0; k < 3; k++)
	{
		tVtx.pos[k] = vtxPos[k];
		tVtx.normal[k] = normal[k];
	}
	//cout << tVtx.pos[0] << ", " << tVtx.pos[1] << ", " << tVtx.pos[2] << endl;
	//cout << ux << ", " << vx << endl;
	tVtx.uv[0] = u;
	tVtx.uv[1] = v;
	
	return tVtx;
}

void uppdateMeshVertices(MFnMesh &myModel, int meshIndex)
{
	vector<Vertice> tempVerteces;
	vector<VertexIndex> tempVtxIndex;
	VertexIndex vtxIndex;
	MStringArray uvString;
	MFloatArray newU, newV;
	//float u, v;
	//cout << "updated points" << endl;
	for (int i = 0; i < myModel.numPolygons(); i++)
	{
		MIntArray myIntArray;
		myModel.getPolygonVertices(i, myIntArray);
		int* vertexIDCurrentFace = new int[myIntArray.length()];
		myIntArray.get(vertexIDCurrentFace);
		myModel.getUVSetNames(uvString);
		myModel.getUVs(newU, newV, &uvString[0]);
		int uvId;
		for (int j = 0; j < myIntArray.length(); j++)
		{
			if (j < 3)
			{
				//myModel.getPolygonUV(i, vertexIDCurrentFace[j], u, v, uvString);
				//beter Whay? 
				myModel.getPolygonUVid(i, j, uvId, &uvString[0]);
				//cout << "vtxIndex: " << vertexIDCurrentFace[j] << endl;
				//cout << "j: " << j << endl;
				//cout << "uv id: " << uvId << endl;
				//cout << "new uvs: " << newU[uvId] << ", " << newV[uvId] << endl;
				//calcVtx(myModel, vertexIDCurrentFace[j], i, 1, 1);

				//myModel.getPolygonUVid(i, vertexIDCurrentFace[j], uvId, uvString);
				//cout << "number of uvs: " << myModel.numUVs(&status) << endl;
				tempVerteces.push_back(calcVtx(myModel, vertexIDCurrentFace[j], i, newU[uvId], newV[uvId]));
				vtxIndex.index = (tempVerteces.size() - 1);
				tempVtxIndex.push_back(vtxIndex);
				//cout << u << ", " << v <<endl;
				//myModel.getUV(uvId, u, v, uvString);
				//cout << u << ", " << v << endl;
				//cout << vtxIndex.index << endl;
				//cout << "" << endl;
			}
			else
			{
				myModel.getPolygonUVid(i, j, uvId, &uvString[0]);
				//cout << "uv id: " << uvId << endl;
				//cout << "new uvs: " << newU[uvId] << ", " << newV[uvId] << endl;
				//calcVtx(myModel, vertexIDCurrentFace[j], i, 1, 1);
				//myModel.getPolygonUV(i, vertexIDCurrentFace[j], u, v, uvString);
				tempVerteces.push_back(calcVtx(myModel, vertexIDCurrentFace[j], i, newU[uvId], newV[uvId]));
				//cout << u << ", " << v << endl;
				//cout << "###" << endl;
				//vtxIndex.index = (vertexIDCurrentFace[0]);
				//tempVtxIndex.push_back(vtxIndex);
				//cout << vtxIndex.index << endl;

				//vtxIndex.index = (vertexIDCurrentFace[j-1]);
				//tempVtxIndex.push_back(vtxIndex);
				//cout << vtxIndex.index << endl;

				//vtxIndex.index = (vertexIDCurrentFace[j]);
				//tempVtxIndex.push_back(vtxIndex);
				//cout << vtxIndex.index << endl;
				//cout << "###" << endl;
				//cout << "" << endl;
				vtxIndex.index = (tempVerteces.size() - 1 - j);
				tempVtxIndex.push_back(vtxIndex);
				//cout << vtxIndex.index << endl;
				//cout << "" << endl;

				vtxIndex.index = (tempVerteces.size() - 2);
				tempVtxIndex.push_back(vtxIndex);
				//cout << vtxIndex.index << endl;
				//cout << "" << endl;

				vtxIndex.index = (tempVerteces.size() - 1);
				tempVtxIndex.push_back(vtxIndex);
				//cout << vtxIndex.index << endl;
				//cout << "" << endl;
			}
		}
		delete[] vertexIDCurrentFace;
	}
	//cout << myModel.numVertices() << endl;
	//cout << myModel.numFaceVertices() << endl;
	int isNew = m_meshes.at(meshIndex).verticeCount;
	m_meshVtx.at(meshIndex).meshVtxs = tempVerteces;
	m_meshVtxIndex.at(meshIndex).meshVtxIndex = tempVtxIndex;
	m_meshes.at(meshIndex).verticeCount = tempVerteces.size();
	m_meshes.at(meshIndex).indexCount = tempVtxIndex.size();

	if (isNew > 0)
	{
		int spot = -1;
		for (int k = 0; k < m_changeMeshID.size(); k++)
		{
			if (m_changeMeshID.at(k).id == meshIndex)
				spot = k;
		}
		if (spot == -1)
		{
			ChangeMesh cm_Temp;
			cm_Temp.id = meshIndex;
			cm_Temp.vtx = true;
			m_changeMeshID.push_back(cm_Temp);
			m_SendHeder.meshChange++;
		}
		else
			m_changeMeshID.at(spot).id = meshIndex;
	}
	else if (isNew == 0)
	{
		for (int i = 0; i <= myModel.name().length(); i++)
		{
			m_meshes.at(meshIndex).name[i] = myModel.name().asChar()[i];
		}
		Material tempMat = getMaterial(myModel);
		int tempId = m_meshMaterial.size();
		for (int i = 0; i < m_meshMaterial.size(); i++)
		{
			//cout << "new name: " << string(tempMat.name) << " alredy existing name: "<< string(m_meshMaterial.at(i).name) << endl;
			if (string(tempMat.name) == string(m_meshMaterial.at(i).name))
			{
				//cout << "material alrady exist " << endl;
				tempId = i;
			}
		}
		if (tempId == m_meshMaterial.size())
		{
			unsigned int instensNr = 0;
			MObjectArray shaders;
			MIntArray shaderIndices;
			myModel.getConnectedShaders(instensNr, shaders, shaderIndices);
			MObject shaderObject[3];
			shaders.get(shaderObject);
			MItDependencyGraph shaderIt(shaderObject[0], MFn::kLambert, MItDependencyGraph::kUpstream, MItDependencyGraph::kDepthFirst, MItDependencyGraph::kNodeLevel, &status);
			MObject myShader = shaderIt.thisNode();
			callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(myShader, nodeAttrebuteChange, NULL, &status));
			callbackIdArray.append(MNodeMessage::addNameChangedCallback(myShader, nodeNameChange, NULL, &status));
			m_meshMaterial.push_back(tempMat);
			m_Heder.materialCount++;
			m_SendHeder.newMaterial++;
		}
		m_meshes.at(meshIndex).materialID = tempId;
		m_SendHeder.newMesh++;
		//cout << "new mesh " << m_meshes.at(meshIndex).verticeCount << endl;
	}

}

void timedCheckCallback(float elapsedTime, float lastTime, void*)
{
	//cout << "its time" << endl;
	//cout << "elapsed time " << elapsedTime << endl;
	//cout << lastTime << " the last time " << endl;
	if (meshIdToUpdate != -1)
	{
		//printing all vertices for safty
		//cout << "____________________________________" << endl;
		MFnMesh myModel(allMeshes.at(meshIdToUpdate));
		uppdateMeshVertices(myModel, meshIdToUpdate);
		meshIdToUpdate = -1;
		//cout << "uppdated: " << myModel.name() << " vertices pos" << endl;
	}
}

void dirtyNodePlug(MObject &node, MPlug &plug, void *clientData) {
	char spliter = '.';
	MStringArray tempStringArray;
	MString myString[10];
	
	if (node.hasFn(MFn::kMesh))
	{
		MString tempString = plug.info();
		tempString.split(spliter, tempStringArray);
		tempStringArray.get(myString);
		//MGlobal::displayInfo(myString[1]);
		if (myString[1] == "inMesh")
		{
			MPlugArray myPlugArray;
			MPlug conectedPlugs[10];
			if (plug.connectedTo(myPlugArray, true, false, &status))
			{
				myPlugArray.get(conectedPlugs);
			}
			if(conectedPlugs[0].info().substring(0, 10) != "polyTweakUV")
			{
				for (int i = 0; i < allMeshes.size(); i++)
				{
					MFnMesh temp = allMeshes.at(i);
					MFnMesh plugMesh = node;
					if (temp.name() == plugMesh.name())
					{
						meshIdToUpdate = i;
					}
				}
			}
		}
	}
}

void childTransformPrinter(MFnTransform &node, MMatrix parant )
{
	//cout << "transform chang in child " << node.name() << endl;
	MMatrix myMatrix = node.transformationMatrix() * parant;
	//cout << "new matrix (global): " << endl;
	//cout << myMatrix << endl;
	for (int i = 0; i < node.childCount(); i++)
	{
		if (node.child(i, &status).hasFn(MFn::kTransform))
		{
			MFnTransform childTransform = node.child(i, &status);
			childTransformPrinter(childTransform, myMatrix);
		}
		if (node.child(i, &status).hasFn(MFn::kMesh))
		{
			MFnMesh transformMesh = node.child(i, &status);
			for (int j = 0; j < allMeshes.size(); j++)
			{
				MFnMesh temp = allMeshes.at(j);
				if (temp.name() == transformMesh.name())
				{
					for (int x = 0; x < 4; x++)
						for (int y = 0; y < 4; y++)
							m_meshes.at(j).meshTransform.TransformMatrix[x][y] = myMatrix[x][y];

					int spot = -1;
					for (int k = 0; k < m_changeMeshID.size(); k++)
					{
						if (m_changeMeshID.at(k).id == j)
							spot = k;
					}
					if (spot == -1)
					{
						ChangeMesh cm_Temp;
						cm_Temp.id = j;
						m_changeMeshID.push_back(cm_Temp);
						m_SendHeder.meshChange++;
					}
					else
						m_changeMeshID.at(spot).id = j;
				}
			}
		}
	}
}

MMatrix getGlobalTransform(MObject &node)
{
	//cout << "---" << endl;
	if (node.hasFn(MFn::kDagNode))
	{
		MFnDagNode myDagNode = node;
		MMatrix myMatrix;
		MMatrix returnMatrix;
		//cout << myDagNode.parentCount() << endl;
		if (myDagNode.parentCount() == 1)
			myMatrix = getGlobalTransform(myDagNode.parent(0, &status));

		if (node.hasFn(MFn::kTransform))
		{
			//cout << "is transform " << endl;
			MFnTransform myTransform = node;
			//returnMatrix = myTransform.transformationMatrix();
			//returnMatrix *= myMatrix;
			//myMatrix = returnMatrix;
			myMatrix = myTransform.transformationMatrix() * myMatrix;
			//cout << "is transform: " << myMatrix << endl;
		}
		//else
		//	cout << "is not transform " << endl;

		return myMatrix;
	}
}

void nodeAttrebuteChange(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void*) {
	gTimer.endTimer();

		//cout << "somthing changed" << endl;
		////cout << "somthing conected" << endl;
		//cout << plug.info() << " " << endl;
		//cout << "Type: " << plug.node().apiTypeStr() << endl;
		//cout << "Type: " << msg << endl;
		//cout << "Type: " << MNodeMessage::AttributeMessage::kOtherPlugSet << endl;
		//cout << otherPlug.info() << endl;
		//cout << otherPlug.node().apiTypeStr() << endl;
	if (plug.node().hasFn(MFn::kLambert))
	{
		Material tempMaterial;
		MFnLambertShader testLambert = plug.node();
		//cout << "shader name? " << testLambert.name() << endl;
		for (int i = 0; i <= testLambert.name().length(); i++)
		{
			tempMaterial.name[i] = testLambert.name().asChar()[i];
		}

		//cout << "shader color: " << testLambert.color() << endl;
		tempMaterial.color[0] = testLambert.color().r;
		tempMaterial.color[1] = testLambert.color().g;
		tempMaterial.color[2] = testLambert.color().b;


		//get the diffuse/color textur
		MFnDependencyNode fn(plug.node());
		MPlug colorPlug = fn.findPlug("color");
		MPlugArray myColorPlugs;
		colorPlug.connectedTo(myColorPlugs, true, false);
		for (int i = 0; i < myColorPlugs.length(); i++)
		{
			//cout << "a textur?" << endl;
			if (myColorPlugs[i].node().hasFn(MFn::kFileTexture))
			{
				cout << "I found a textur!" << endl << "it is a diffuse map" << endl;
				MFnDependencyNode textur(myColorPlugs[i].node());
				MPlug texturNamePlug = textur.findPlug("fileTextureName");
				MString myFilePath;
				texturNamePlug.getValue(myFilePath);
				cout << myFilePath << endl;
				for (int j = 0; j < myFilePath.length(); j++)
					tempMaterial.diffuseMap[j] = myFilePath.asChar()[j];
				cout << tempMaterial.diffuseMap << endl;
			}
		}

		//get the normal txture
		//MFnDependencyNode fn(myShader);
		MPlug bumpPlug = fn.findPlug("normalCamera");
		MPlugArray myBumpPlugs;
		bumpPlug.connectedTo(myBumpPlugs, true, false);
		for (int i = 0; i < myBumpPlugs.length(); i++)
		{
			//cout << "a conection" << endl;
			MFnDependencyNode bumpTexturNode(myBumpPlugs[i].node());
			//cout << "node name " << bumpTexturNode.name() << endl;
			MPlug bumpTxPlug = bumpTexturNode.findPlug("bumpValue");
			MPlugArray myBumpTxs;
			bumpTxPlug.connectedTo(myBumpTxs, true, false);
			for (int j = 0; j < myBumpTxs.length(); j++)
			{
				//cout << "a textur?" << endl;
				if (myBumpTxs[j].node().hasFn(MFn::kFileTexture))
				{
					//cout << "I found a textur!" << endl << "it is a normal map" << endl;
					MFnDependencyNode textur(myBumpTxs[j].node());
					MPlug texturNamePlug = textur.findPlug("fileTextureName");
					MString myFilePath;
					texturNamePlug.getValue(myFilePath);
					//cout << myFilePath << endl;
				}
			}
		}

		//get the speculer textur?
		//-----
		// add here
		//----


		//vi kör här inne för färg ändringar
		for (int j = 0; j < m_meshMaterial.size(); j++)
		{
			//cout << "somthing changed" << endl;
			//cout << string(tempMaterial.name) << endl;
			//cout << string(m_meshMaterial.at(j).name) << endl;
			if (string(tempMaterial.name) == string(m_meshMaterial.at(j).name))
			{
				m_meshMaterial.at(j) = tempMaterial;
				int spot = -1;
				for (int k = 0; k < m_changeMaterialID.size(); k++)
				{
					if (m_changeMaterialID.at(k) == j)
						spot = k;
				}
				if (spot == -1)
				{
					m_changeMaterialID.push_back(j);
					m_SendHeder.materialChange++;
				}
				else
					m_changeMaterialID.at(spot) = j;
			}
		}

	}

	if (msg & MNodeMessage::AttributeMessage::kOtherPlugSet)
	{
		if (plug.node().hasFn(MFn::kMesh) && string(otherPlug.node().apiTypeStr()) == "kShadingEngine")
		{
			//newMaterial
			MFnMesh myMesh = plug.node();
			Material tempMat = getMaterial(myMesh);
			int tempId = m_meshMaterial.size();
			for (int i = 0; i < m_meshMaterial.size(); i++)
			{
				//cout << "new name: " << string(tempMat.name) << " alredy existing name: " << string(m_meshMaterial.at(i).name) << endl;
				if (string(tempMat.name) == string(m_meshMaterial.at(i).name))
				{
					//cout << "material alrady exist " << endl;
					tempId = i;
				}
			}
			if (tempId == m_meshMaterial.size())
			{
				unsigned int instensNr = 0;
				MObjectArray shaders;
				MIntArray shaderIndices;
				myMesh.getConnectedShaders(instensNr, shaders, shaderIndices);
				MObject shaderObject[3];
				shaders.get(shaderObject);
				MItDependencyGraph shaderIt(shaderObject[0], MFn::kLambert, MItDependencyGraph::kUpstream, MItDependencyGraph::kDepthFirst, MItDependencyGraph::kNodeLevel, &status);
				MObject myShader = shaderIt.thisNode();
				callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(myShader, nodeAttrebuteChange, NULL, &status));
				callbackIdArray.append(MNodeMessage::addNameChangedCallback(myShader, nodeNameChange, NULL, &status));
				m_meshMaterial.push_back(tempMat);
				m_Heder.materialCount++;
				m_SendHeder.newMaterial++;
			}
			for (int j = 0; j < allMeshes.size(); j++)
			{
				MFnMesh temp = allMeshes.at(j);
				if (temp.name() == myMesh.name())
				{
					m_meshes.at(j).materialID = tempId;
					int spot = -1;
					for (int k = 0; k < m_changeMeshID.size(); k++)
					{
						if (m_changeMeshID.at(k).id == j)
							spot = k;
					}
					if (spot == -1)
					{
						ChangeMesh cm_Temp;
						cm_Temp.id = j;
						cm_Temp.vtx = true;
						m_changeMeshID.push_back(cm_Temp);
						m_SendHeder.meshChange++;
					}
					else
						m_changeMeshID.at(spot).id = j;
				}
			}

			//cout << "it is in!" << endl;
		}
	}
	//	if (plug.node().hasFn(MFn::kMesh) && string(otherPlug.node().apiTypeStr()) == "kShadingEngine")
	//	{
	//		cout << "it is in!" << endl;
	//		cout << "somthing changed" << endl;
	//		//cout << "somthing conected" << endl;
	//		cout << plug.info() << " " << endl;
	//		cout << "Type: " << plug.node().apiTypeStr() << endl;
	//		cout << "Type: " << msg << endl;
	//		cout << "Type: " << MNodeMessage::AttributeMessage::kOtherPlugSet << endl;
	//		cout << otherPlug.info() << endl;
	//		cout << otherPlug.node().apiTypeStr() << endl;
	//	}

	if (plug.node().apiType() == MFn::kCamera)
	{
		//cout << "is a camera: " << endl;
		MFnCamera myCamera = plug.node();
		//cout << myCamera.name() << endl;

		for (int i = 0; i < allCameras.size(); i++)
		{
			MFnCamera temp = allCameras.at(i);
			if (temp.name() == myCamera.name())
			{
				int whith = 0, hight = 0;
				double hFOV = 0, vFOV = 0;
				myCamera.getPortFieldOfView(whith, hight, hFOV, vFOV);
				double oWidth = myCamera.orthoWidth(&status);
				//cout << oWidth << endl;
				m_Cameras.at(i).oWhith = oWidth;
				//cout << myCamera.cameraScale(&status) << endl;
				//cout << myCamera.isOrtho() << endl;
				m_Cameras.at(i).isOrtho = myCamera.isOrtho();

				m_Cameras.at(i).FOV = hFOV * (180 / 3.1415926);
				m_Cameras.at(i).Far = myCamera.farClippingPlane();
				m_Cameras.at(i).Ner = myCamera.nearClippingPlane();
				m_Cameras.at(i).aspectRatio = myCamera.aspectRatio();
				if (myCamera.parentCount() == 1)
				{
					MObject tempTransform = myCamera.parent(0, &status);
					MMatrix tempTMatrix = getGlobalTransform(tempTransform);
					for (int i = 0; i < 4; i++)
						for (int j = 0; j < 4; j++)
							m_Cameras.at(i).cameraTransform.TransformMatrix[i][j] = tempTMatrix[i][j];
				}

				int spot = -1;
				for (int j = 0; j < m_changeCameraID.size(); j++)
				{
					if (m_changeCameraID[j] == i)
						spot = j;
				}
				if (spot == -1)
				{
					m_changeCameraID.push_back(i);
					m_SendHeder.cameraChange++;
				}
				else
					m_changeCameraID.at(spot) = i;
			}
		}
	}

	if (plug.node().hasFn(MFn::kTransform))
	{
		if (msg & MNodeMessage::AttributeMessage::kAttributeSet)
		{
			MFnTransform myTransform = plug.node();
			//MFnDagNode myDagNode = plug.node();
			// hela transformen!!!!
			MVector myTranslate = myTransform.translation(MSpace::kObject, &status);
			MMatrix myMatrix = myTransform.transformationMatrix();

			//cout << myTransform.childCount() << endl;
			for (int i = 0; i < myTransform.childCount(); i++)
			{

				MMatrix globalTransform = getGlobalTransform(plug.node());
				//cout << "the real trans" << endl;
				if (myTransform.child(i, &status).hasFn(MFn::kCamera))
				{
					//cout << "CAMERA HEAR" << endl;
					MFnCamera transformCamera = myTransform.child(i, &status);
					for (int j = 0; j < allCameras.size(); j++)
					{
						MFnCamera temp = allCameras.at(j);
						if (temp.name() == transformCamera.name())
						{
							//cout << "HEAR" << endl;
							//cout << m_CamerasCurentTransforms.at(j).cameraTransform.TransformMatrix[3][0] << " " << m_CamerasCurentTransforms.at(j).cameraTransform.TransformMatrix[3][1] << endl;
							onlySomeTimes = 0;
							for (int x = 0; x < 4; x++)
								for (int y = 0; y < 4; y++)
								{
									float tempTrnsformValu = globalTransform[x][y];
									if (m_CamerasCurentTransforms.at(j).cameraTransform.TransformMatrix[x][y] != tempTrnsformValu)
									{
										//cout << " the diffrens " << m_CamerasCurentTransforms.at(j).cameraTransform.TransformMatrix[x][y] << ", " << tempTrnsformValu << endl;
										onlySomeTimes = 1;
									}
								}
							//cout << onlySomeTimes << endl;
							if (onlySomeTimes == 1)
							{
								for (int x = 0; x < 4; x++)
									for (int y = 0; y < 4; y++)
										m_Cameras.at(j).cameraTransform.TransformMatrix[x][y] = globalTransform[x][y];

								int spot = -1;
								for (int k = 0; k < m_changeCameraID.size(); k++)
								{
									if (m_changeCameraID[k] == j)
										spot = k;
								}
								if (spot == -1)
								{
									m_changeCameraID.push_back(j);
									m_SendHeder.cameraChange++;
								}
								else
									m_changeCameraID.at(spot) = j;
							}

						}

					}

				}


				if (myTransform.child(i, &status).hasFn(MFn::kMesh))
				{
					MFnMesh transformMesh = myTransform.child(i, &status);
					for (int j = 0; j < allMeshes.size(); j++)
					{
						MFnMesh temp = allMeshes.at(j);
						if (temp.name() == transformMesh.name())
						{
							for (int x = 0; x < 4; x++)
								for(int y = 0; y < 4; y++)
									m_meshes.at(j).meshTransform.TransformMatrix[x][y] = globalTransform[x][y];
						
							int spot = -1;
							for (int k = 0; k < m_changeMeshID.size(); k++)
							{
								if (m_changeMeshID.at(k).id == j)
									spot = k;
							}
							if (spot == -1)
							{
								ChangeMesh cm_Temp;
								cm_Temp.id = j;
								m_changeMeshID.push_back(cm_Temp);
								m_SendHeder.meshChange++;
							}
							else
								m_changeMeshID.at(spot).id = j;
						}
					}
				}
				if (myTransform.child(i, &status).hasFn(MFn::kTransform))
				{
					MFnTransform childTransform = myTransform.child(i, &status);
					childTransformPrinter(childTransform, getGlobalTransform(plug.node()));
					//cout << endl;
				}
			}
		}
	}
	if (plug.node().hasFn(MFn::kMesh))
	{
		if (msg & MNodeMessage::AttributeMessage::kAttributeSet) {
			MFnMesh plugMesh(plug.node());
			//cout << "somthing changed" << endl;
			//cout << "somthing conected" << endl;
			//cout << plug.info() << endl;
			////cout << plugMesh.uv << endl;
			//cout << plug.info().substring(plug.info().length() - 7, plug.info().length()) << endl;
			if (plug.info().substring(plug.info().length()-7, plug.info().length()) == "uvPivot")
			{
				for (int i = 0; i < allMeshes.size(); i++)
				{
					MFnMesh temp = allMeshes.at(i);
					if (temp.name() == plugMesh.name())
					{
						meshIdToUpdate = i;
					}
				}
			}
			if (plug.isElement()) {
				for (int i = 0; i < allMeshes.size(); i++)
				{
					MFnMesh temp = allMeshes.at(i);
					if (temp.name() == plugMesh.name())
					{
						MPoint tempP;
						MVector normal;
						plugMesh.getPoint(plug.logicalIndex(), tempP, MSpace::kObject);
						//plugMesh.getVertexNormal(plug.logicalIndex(), normal, MSpace::kObject);

						float vtxPos[4];
						tempP.get(vtxPos);

						//cout << "new pos for vertice " << plug.logicalIndex() << ":" << endl;

						int vtxCounter = 0;
						for (int k = 0; k < temp.numPolygons(); k++)
						{
							MIntArray myIntArray;
							temp.getPolygonVertices(k, myIntArray);
							int* vertexIDCurrentFace = new int[myIntArray.length()];
							myIntArray.get(vertexIDCurrentFace);
							for (int j = 0; j < myIntArray.length(); j++)
							{
								if (vertexIDCurrentFace[j] == plug.logicalIndex())
								{
									plugMesh.getFaceVertexNormal(k, vertexIDCurrentFace[j], normal, MSpace::kObject);
									for (int l = 0; l < 3; l++)
									{
										m_meshVtx.at(i).meshVtxs.at(vtxCounter).pos[l] = vtxPos[l];
										m_meshVtx.at(i).meshVtxs.at(vtxCounter).normal[l] = normal[l];
										//cout << "vertax change in index: " << vtxCounter << endl;
									}
								}
								vtxCounter++;
							}
						}
						//cout << "" << endl;
						int spot = -1;
						for (int k = 0; k < m_changeMeshID.size(); k++)
						{
							if (m_changeMeshID.at(k).id == i)
								spot = k;
						}
						if (spot == -1)
						{
							ChangeMesh cm_Temp;
							cm_Temp.id = i;
							cm_Temp.vtx = true;
							m_changeMeshID.push_back(cm_Temp);
							m_SendHeder.meshChange++;
						}
						else
							m_changeMeshID.at(spot).id = i;
					}
				}
			}

			//cout << "." << endl;
		}
		//struntar i detta för det behövs inte
		//if (msg & MNodeMessage::AttributeMessage::kConnectionBroken) {
		//	cout << "somthing changed" << endl;
		//	cout << "somthing brok" << endl;
		//	cout << plug.info() << endl;
		//	cout << otherPlug.info() << endl;
		//	cout << otherPlug.asMObject().apiTypeStr() << endl;
		//	if (plug.asMObject().hasFn(MFn::kMesh))
		//	{
		//		for (int i = 0; i < allMeshes.size(); i++)
		//		{
		//			MFnMesh temp = allMeshes.at(i);
		//			MFnMesh plugMesh = plug.node();
		//			if (temp.name() == plugMesh.name())
		//			{
		//				nrOfVtx.at(i) = plugMesh.numVertices();
		//			}
		//		}
		//	}
		//}
	}
	changeTimer += gTimer.elapsedTime();
	//cout << "timer test elapsed: " << gTimer.elapsedTime() << endl;
	//cout << "timer test: " << changeTimer << endl;
	if (changeTimer > 0.01)
	{
		changeTimer = 0;
		sendDataToComLib();
	}
	//lastTime = gTimer.elapsedTime();
	gTimer.beginTimer();
}

void nodeAdded(MObject &node, void * clientData) {
	//... implement this and other callbacks

	//MGlobal::displayInfo("added");

	//cout << "Added somthing to maya ===========================" << endl;
	MFnDagNode myDagNode = node;
	//MFn::Type typOfNode = node.apiType();
	//MFn::kPolyMesh 
	//MFnMesh myMesh = node;
	//cout << myMesh.name() << " was added-----------------" << endl;
	//MGlobal::displayInfo(node.apiTypeStr());
	if (node.hasFn(MFn::kDagNode))
	{
		cout << "===========================" << endl;
		cout << myDagNode.name() << " was added" << endl;
		if (node.hasFn(MFn::kMesh))
		{
			MFnMesh myMesh = node;
			allMeshes.push_back(node);
			//cout << myMesh.numVertices() << endl;
			//nrOfVtx.push_back(myMesh.numVertices());

			m_Heder.meshCount++;
			Mesh tempMesh;
			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
				{
					if (j == i)
						tempMesh.meshTransform.TransformMatrix[i][j] = 1;
					else
						tempMesh.meshTransform.TransformMatrix[i][j] = 0;
				}

			m_meshes.push_back(tempMesh);
			MeshVertices tempVtx;
			m_meshVtx.push_back(tempVtx);
			MeshVertexIndex tempVtxIndex;;
			m_meshVtxIndex.push_back(tempVtxIndex);

		}
		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(node, nodeAttrebuteChange, NULL, &status));
		callbackIdArray.append(MNodeMessage::addNameChangedCallback(node, nodeNameChange, NULL, &status));
		callbackIdArray.append(MNodeMessage::addNodeDirtyPlugCallback(node, dirtyNodePlug, NULL, &status));
		//cout << "the type is: " << node.apiTypeStr() << endl;
	}
	//else
	//	cout << node.apiTypeStr() << " was added" << endl << "it is not a dag node so don't know if it has a name" << endl;
	//cout << "the type is: " << node.apiTypeStr() << endl;
	//cout << "===========================" << endl;
}
void nodeRemoved(MObject &node, void * clientData) {
	//MGlobal::displayInfo("removed");
	MFnDagNode myDagNode = node;

	//MGlobal::displayInfo(node.apiTypeStr());
	if (node.hasFn(MFn::kDagNode))
	{
		cout << "===========================" << endl;
		cout << myDagNode.name() << " was removed" << endl;

		if (node.hasFn(MFn::kMesh))
		{
			MFnMesh myMesh = node;
			for (int i = 0; i < allMeshes.size(); i++)
			{
				MFnMesh temp = allMeshes.at(i);
				if (temp.name() == myMesh.name())
				{
					m_meshes.at(i).materialID = -1;
					MObject emty;
					allMeshes.at(i) = emty;
					int spot = -1;
					for (int j = 0; j < m_changeMeshID.size(); j++)
					{
						if (m_changeMeshID.at(j).id == i)
							spot = j;
					}
					if (spot == -1)
					{
						ChangeMesh cm_Temp;
						cm_Temp.id = i;
						cm_Temp.vtx = true;
						m_changeMeshID.push_back(cm_Temp);
						m_SendHeder.meshChange++;
					}
					else
						m_changeMeshID.at(spot).id = i;

				}
			}
		}
	}
	//else
	//	cout << node.apiTypeStr() << " was removed" << endl << "it is not a dag node so don't know if it has a name" << endl;
	//cout << "the type is: " << node.apiTypeStr() << endl;
}
//void nodeChangedTest(MObject &node, void * clientData) {
//	//... implement this and other callbacks
//	MGlobal::displayInfo("changed?");
//	cout << "somthing got changed ===========================" << endl;
//}


/*
 * Plugin entry point
 * For remote control of maya
 * open command port: commandPort -nr -name ":1234"
 * close command port: commandPort -cl -name ":1234"
 * send command: see loadPlugin.py and unloadPlugin.py
 */
EXPORT MStatus initializePlugin(MObject obj) {

	MStatus res = MS::kSuccess;

	MFnPlugin myPlugin(obj, "level editor", "1.0", "Any", &res);

	if (MFAIL(res)) {
		CHECK_MSTATUS(res);
		return res;
	}  

	// redirect cout to cerr, so that when we do cout goes to cerr
	// in the maya output window (not the scripting output!)
	std::cout.set_rdbuf(MStreamUtils::stdOutStream().rdbuf());
	std::cerr.set_rdbuf(MStreamUtils::stdErrorStream().rdbuf());
	cout << "Plugin loaded ===========================" << endl;
	MGlobal::displayInfo("Plugin loaded");

	//MGlobal::displayInfo("hej");
	//MGlobal::displayInfo("hej");

	//MGlobal::displayInfo("maya have these objets already:");
	MItDag meshIt(MItDag::kBreadthFirst, MFn::kMesh, &res);
	for (; !meshIt.isDone(); meshIt.next()) {
		MFnMesh myMeshFn(meshIt.currentItem());
		//MGlobal::displayInfo(myMeshFn.name());
	}

	//MMessage::MNodeFunction nodedAdded = nodeAdded;
	//MMessage::MNodeFunction nodedRemoved = nodeRemoved;
	//MMessage::MNodeFunction nodeChangedTest =
	// register callbacks here for
	callbackIdArray.append(MDGMessage::addNodeAddedCallback(nodeAdded, "dependNode", NULL, &status));
	callbackIdArray.append(MDGMessage::addNodeRemovedCallback(nodeRemoved, "dependNode", NULL, &status));
	callbackIdArray.append(MTimerMessage::addTimerCallback(1000, timerCallback, NULL, &status));
	callbackIdArray.append(MTimerMessage::addTimerCallback(0.1, timedCheckCallback, NULL, &status));
	callbackIdArray.append(MTimerMessage::addTimerCallback(0.1, sendToComLib, NULL, &status));
	//callbackIdArray.append(MDGMessage::addNodeChangeUuidCheckCallback(nodeChangedTest, "dependNode", NULL, &status));
	MItDag dagIt(MItDag::kDepthFirst, MFn::kDagNode, &res);
	meshIt.reset();
	//MGlobal::displayInfo("test to gett ReName");
	for (; !dagIt.isDone(); dagIt.next()){
		MFnMesh myMeshFn(dagIt.currentItem());
		//MGlobal::displayInfo(myMeshFn.name());
		m_node = dagIt.currentItem();
		//cout << m_node.apiTypeStr() << endl;
		if (m_node.hasFn(MFn::kCamera))
		{
			allCameras.push_back(m_node);
			Camera tempCamera;
			//cout << "is a camera: " << endl;
			MFnCamera myCamera = m_node;
			//float upVec[3];
			//float direction[3];
			//float FOV, Ner, Far;
			//Trasform cameraTransform;
			int whith = 0, hight  = 0;
			double hFOV = 0, vFOV = 0;
			myCamera.getPortFieldOfView(whith, hight, hFOV, vFOV);
			//cout << whith << " " << hight << " " << hFOV*(180/3.1415926) << " " << vFOV << " " << endl;
			double oWidth = myCamera.orthoWidth(&status);
			//cout << oWidth << endl;
			tempCamera.oWhith = oWidth;
			//cout << myCamera.cameraScale(&status) << endl;
			//cout << myCamera.isOrtho()<< endl;
			tempCamera.isOrtho = myCamera.isOrtho();
			//myCamera.getViewingFrustum();
			//myCamera.upDirection(MSpace::kObject, &status);
			
			//cout << myCamera.shutterAngle(&status) << endl;
			//cout << myCamera.farClippingPlane() << endl;
			//cout << myCamera.nearClippingPlane() << endl;
			//cout << myCamera.farFocusDistance() << endl;
			tempCamera.FOV = hFOV * (180 / 3.1415926);
			tempCamera.Far = myCamera.farClippingPlane();
			tempCamera.Ner = myCamera.nearClippingPlane();
			tempCamera.aspectRatio = myCamera.aspectRatio();
			if (myCamera.parentCount() == 1)
			{
				MObject tempTransform = myCamera.parent(0, &status);
				MMatrix tempTMatrix = getGlobalTransform(tempTransform);
				for (int i = 0; i < 4; i++)
					for (int j = 0; j < 4; j++)
						tempCamera.cameraTransform.TransformMatrix[i][j] = tempTMatrix[i][j];
			}
			tempCamera.id = m_Cameras.size();
			m_Cameras.push_back(tempCamera);
			m_Heder.cameraCount++;
			m_SendHeder.newCamera++;   

			m_CamerasCurentTransforms.push_back(tempCamera);
		}
		if (m_node.hasFn(MFn::kMesh))
		{
			allMeshes.push_back(m_node);
			//nrOfVtx.push_back(myMeshFn.numVertices());

			m_Heder.meshCount++;
			Mesh tempMesh;

			if (myMeshFn.parentCount() == 1)
			{
				MObject tempTransform = myMeshFn.parent(0, &status);
				MMatrix tempTMatrix = getGlobalTransform(tempTransform);
				for (int i = 0; i < 4; i++)
					for (int j = 0; j < 4; j++)
						tempMesh.meshTransform.TransformMatrix[i][j] = tempTMatrix[i][j];

				//cout << myMeshFn.parent(0, &status).apiTypeStr() << endl;
			}

			for (int i = 0; i <= myMeshFn.name().length(); i++)
			{
				tempMesh.name[i] = myMeshFn.name().asChar()[i];
			}

			MeshVertices tempVtx;
			MeshVertexIndex tempVtxIndex;
			m_meshVtx.push_back(tempVtx);
			m_meshVtxIndex.push_back(tempVtxIndex);
			m_meshes.push_back(tempMesh);
			uppdateMeshVertices(myMeshFn, m_meshes.size()-1);

			//sen camra här med
		}

		callbackIdArray.append(MNodeMessage::addAttributeChangedCallback(m_node, nodeAttrebuteChange, NULL, &status));
		callbackIdArray.append(MNodeMessage::addNameChangedCallback(m_node, nodeNameChange, NULL, &status));
		callbackIdArray.append(MNodeMessage::addNodeDirtyPlugCallback(m_node, dirtyNodePlug, NULL, &status));
		//callbackIdArray.append(MNodeMessage::addAttributeAddedOrRemovedCallback(m_node, nodeAttrebuteAddedOrRemoved, NULL, &status));
	}
	sendDataToComLib();
	//MNodeMessage::addAttributeChangedCallback(m_node, nodeAttrebuteChange, NULL, &status);
	//MNodeMessage::addNodeDirtyCallback()

	//MDGMessage::addNodeAddedCallback(nodedAdded, "dependNode", NULL, &status);
	// MDGMessage::addNodeRemovedCallback(nodeRemoved, "dependNode", NULL, &status);
	// MTimerMessage::addTimerCallback(0.1, timerCallback, NULL, &status);

	// a handy timer, courtesy of Maya
	gTimer.clear();
	gTimer.beginTimer();

	return res;
}
	

EXPORT MStatus uninitializePlugin(MObject obj) {
	MFnPlugin plugin(obj);

	cout << "Plugin unloaded =========================" << endl;

	MMessage::removeCallbacks(callbackIdArray);

	return MS::kSuccess;
}