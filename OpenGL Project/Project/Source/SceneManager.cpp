///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"../../Utilities/textures/granite1.jpg",
		"counter");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/tiles.jpg",
		"wall");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/Rubiks_white.jpg",
		"box_white");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/Rubiks_red.jpg",
		"box_red");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/Rubiks_blue.jpg",
		"box_blue");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/cardboard.jpg",
		"paperbag");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/wood.jpg",
		"bagclip");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/bagtag.jpg",
		"label");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/wax.jpg",
		"candlewax");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/ceramic.jpg",
		"mug");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}
 

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.7f, 0.7f, 0.7f);
	glassMaterial.ambientStrength = 0.4f;
	glassMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	glassMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL ceramicMaterial;
	ceramicMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	ceramicMaterial.ambientStrength = 0.5f;
	ceramicMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	ceramicMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	ceramicMaterial.shininess = 25.0;
	ceramicMaterial.tag = "ceramic";

	m_objectMaterials.push_back(ceramicMaterial);

	OBJECT_MATERIAL cardboardMaterial;
	cardboardMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cardboardMaterial.ambientStrength = 0.2f;
	cardboardMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cardboardMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	cardboardMaterial.shininess = 0.0;
	cardboardMaterial.tag = "cardboard";

	m_objectMaterials.push_back(cardboardMaterial);

 }

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// tell shaders to render 
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	
	// There are three lights in this scene, each representing an overhead light that exists in the orginally photogrpahed scene.
	// General light source center and above scene. Bright white light 
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 10.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 5.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 5.0f);


	// Second light source left and above in scene. Softer yellow light.
	m_pShaderManager->setVec3Value("lightSources[1].position", -10.0f, 10.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 1.0f, 0.95f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 0.95f, 0.6f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 15.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 5.0f);
	 
	// Third light source right and above in scene. Softer yellow light
	m_pShaderManager->setVec3Value("lightSources[2].position", 10.0f, 10.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 1.0f, 0.95f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 1.0f, 0.95f, 0.6f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 15.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 5.0f);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();
	// 
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPrismMesh();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderCounter();
	RenderWall();
	RenderMug();
	RenderCandle();
	RenderBag();
	RenderRubiks();
}

void SceneManager::RenderCounter()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh (Plane)
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("counter");
	SetShaderMaterial("ceramic"); 
	SetTextureUVScale(2.0, 2.0);  // UV Scale modified so image is not stretched and appears more like original photo

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}
	
void SceneManager::RenderMug()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh (Tapered Cylinder)
	scaleXYZ = glm::vec3(2.5f, 3.0f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 2.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5f, 0.5f, 0.5f, 0.9);
	SetShaderMaterial("ceramic");  // Material provides a dull sheen, similar to glazed ceramic
	SetShaderTexture("mug");       // Grey Mug image
	SetTextureUVScale(1.0, 1.0);   // No stretching or tiling

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// set the XYZ scale for the mesh (Mug Lip), Torus used to give the "ring-like" shape at the top of the mug
	scaleXYZ = glm::vec3(2.0f, 2.0f, 1.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 2.50f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5f, 0.5f, 0.5f, 0.9);
	SetShaderMaterial("ceramic"); // Matching texture of other mug compoents
	SetShaderTexture("mug");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	// set the XYZ scale for the mesh (Torus), stretched torus used to create the oval of the mug handle
	scaleXYZ = glm::vec3(1.8f, 0.9f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -30.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-8.9f, 1.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5f, 0.5f, 0.5f, 0.9);
	SetShaderMaterial("ceramic"); // Matching texture of other mug compoents
	SetShaderTexture("mug");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
}

void SceneManager::RenderCandle()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	/*** Set needed transformations before drawing the basic mesh ***/

	// set the XYZ scale for the mesh (Candle Wax)
	scaleXYZ = glm::vec3(1.95f, 3.25f, 1.95f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.95f, 0.95f, 0.95f, 1.0f);
	SetShaderTexture("candlewax"); // using the shader color was close but the wax image worked better
	SetTextureUVScale(1.0, 1.0);
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh (Candle Wick)
	scaleXYZ = glm::vec3(.1f, .5f, .1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 3.25f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color for the wick

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh (Candle Bottom)
	scaleXYZ = glm::vec3(2.0f, 3.75f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.7f, 0.7f, 0.8f, 0.3f);
	SetShaderMaterial("glass"); // Glass-like sheen and transparency so the interior candle wax shows through

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh (Candle Bottom Taper)
	scaleXYZ = glm::vec3(2.0f, 0.5f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 3.74f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.7f, 0.7f, 0.8f, 0.3f); // This part of the candle needs to resemble clear glass
	SetShaderMaterial("glass"); 

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// set the XYZ scale for the mesh (Candle Narrow)
	scaleXYZ = glm::vec3(1.75f, 0.85f, 1.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 4.1f, 0.0f);


	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.95f, 0.95f, 0.95f, 0.7f); // This part of the caandle is glass but there is more white in it 
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// set the XYZ scale for the mesh (Candle Lip)
	scaleXYZ = glm::vec3(1.75f, 0.5f, 1.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 4.95f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.7f, 0.7f, 0.8f, 0.3f); // Clear glass
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// set the XYZ scale for the mesh (Candle Lid)
	scaleXYZ = glm::vec3(1.0f, 1.0f, 3.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 5.40f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.7f, 0.7f, 0.8f, 0.3f); // Clear glass
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
}

void SceneManager::RenderRubiks()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh (Plane)
	scaleXYZ = glm::vec3(2.00f, 2.00f, 2.00f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.0f, 1.0f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(.25f, .25f, .25f, 1);
	SetShaderTexture("box_white"); // White rubiks cube pattern used as the base color for the cube
	// No shader material used here as plastic faces from texture work perfectly
	SetTextureUVScale(1.0, 1.0);
	

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the mesh (Plane)
	scaleXYZ = glm::vec3(1.00f, 1.00f, 1.00f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 135.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.75f, 1.0f, 2.71f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(.25f, .25f, .25f, 1);
	SetShaderTexture("box_red"); // Red rubiks cube pattern, picture scaled to shape size to prevent stretching 
	SetTextureUVScale(1.0, 1.0);


	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	
	// set the XYZ scale for the mesh (Plane)
	scaleXYZ = glm::vec3(1.00f, 1.00f, 1.00f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.0f, 2.01f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(.25f, .25f, .25f, 1);
	SetShaderTexture("box_blue"); // Bluerubiks cube pattern, picture scaled to shape size to prevent stretching
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

} 

void SceneManager::RenderBag()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// set the XYZ scale for the mesh (Box)
	scaleXYZ = glm::vec3(3.95f, 2.60f, 2.30f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.5f, 1.25f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5f, 0.5f, 0.5f, 1);
	SetShaderTexture("paperbag"); // Image of cardboard used to achieve a paperbag texture
	SetShaderMaterial("cardboard");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// set the XYZ scale for the mesh (Prism)
	scaleXYZ = glm::vec3(2.35f, 3.95f, 4.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -110.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.5f, 4.55f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.25f, 0.25f, 0.25f, 1);
	SetShaderTexture("paperbag");  // Image of cardboard used to achieve a paperbag texture
	SetShaderMaterial("cardboard");
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();

	// set the XYZ scale for the mesh (Label)
	scaleXYZ = glm::vec3(1.2f, 0.0f, 1.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 78.0f;
	YrotationDegrees = -5.0f;
	ZrotationDegrees = 20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.2f, 4.5f, 0.65f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0);
	SetShaderTexture("label");  // Actual image of label on package used 
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// set the XYZ scale for the mesh (Bag Clip1)
	scaleXYZ = glm::vec3(0.4f, 0.0f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 78.0f;
	YrotationDegrees = -5.0f;
	ZrotationDegrees = 20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.9f, 6.25f, -0.3f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5f, 0.5f, 0.5f, 1);
	SetShaderTexture("bagclip");  // A different shade of cardbaord is used for the bag clips to provide differentiation in the textures
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// set the XYZ scale for the mesh (Bag Clip2)
	scaleXYZ = glm::vec3(0.4f, 0.0f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 78.0f;
	YrotationDegrees = -5.0f;
	ZrotationDegrees = 20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.91f, 6.25f, 0.72f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5f, 0.5f, 0.5f, 1);
	SetShaderTexture("bagclip"); // A different shade of cardbaord is used for the bag clips to provide differentiation in the textures
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}

void SceneManager::RenderWall()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh (Plane)
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 5.0f, -4.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("ceramic");
	SetShaderTexture("wall"); // Image of tile wall with tiles offset
	SetTextureUVScale(1.0, 1.0);

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}

