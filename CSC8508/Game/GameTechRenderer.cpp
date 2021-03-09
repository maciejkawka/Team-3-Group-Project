#include "GameTechRenderer.h"
#include "../Engine/GameObject.h"
#include "../../Common/Camera.h"
#include "../../Common/Vector2.h"
#include "../../Common/Vector3.h"
#include "../../Common/TextureLoader.h"
#include "../../Common/Maths.h"
using namespace NCL;
using namespace Rendering;
using namespace CSC8508;

#define SHADOWSIZE 4096

Matrix4 biasMatrix = Matrix4::Translation(Vector3(0.5, 0.5, 0.5)) * Matrix4::Scale(Vector3(0.5, 0.5, 0.5));

GameTechRenderer::GameTechRenderer(GameWorld& world, ResourceManager& resourceManager) : OGLRenderer(*Window::GetWindow()), gameWorld(world) {
	glEnable(GL_DEPTH_TEST);

	shadowShader = (OGLShader*)resourceManager.LoadShader("GameTechShadowVert.glsl", "GameTechShadowFrag.glsl");

	m_temp_shader = (OGLShader*)resourceManager.LoadShader("gameTechVert.glsl", "gameTechFrag.glsl");
	//shader->GetProgramID();
	shader = (OGLShader*)resourceManager.LoadShader("GameTechVert.glsl", "GameTechFrag.glsl");

	// 设置阴影
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(1, 1, 1, 1);

	//Set up the light properties
	lightColour = Vector4(0.8f, 0.8f, 0.5f, 1.0f);
	lightRadius = 1000.0f;
	lightPosition = Vector3(-200.0f, 60.0f, -200.0f);

	//Skybox!
	skyboxShader = (OGLShader*)resourceManager.LoadShader("skyboxVertex.glsl", "skyboxFragment.glsl");
	skyboxMesh = new OGLMesh();
	skyboxMesh->SetVertexPositions({ Vector3(-1, 1,-1), Vector3(-1,-1,-1) , Vector3(1,-1,-1) , Vector3(1,1,-1) });
	skyboxMesh->SetVertexIndices({ 0,1,2,2,3,0 });
	skyboxMesh->UploadToGPU();

	LoadSkybox();
}

GameTechRenderer::~GameTechRenderer() {

	delete skyboxMesh;

	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
}

void GameTechRenderer::LoadSkybox() {
	string filenames[6] = {
		"/Cubemap/skyrender0004.png",
		"/Cubemap/skyrender0001.png",
		"/Cubemap/skyrender0003.png",
		"/Cubemap/skyrender0006.png",
		"/Cubemap/skyrender0002.png",
		"/Cubemap/skyrender0005.png"
	};

	int width[6] = { 0 };
	int height[6] = { 0 };
	int channels[6] = { 0 };
	int flags[6] = { 0 };

	vector<char*> texData(6, nullptr);

	for (int i = 0; i < 6; ++i) {
		TextureLoader::LoadTexture(filenames[i], texData[i], width[i], height[i], channels[i], flags[i]);
		if (i > 0 && (width[i] != width[0] || height[0] != height[0])) {
			std::cout << __FUNCTION__ << " cubemap input textures don't match in size?\n";
			return;
		}
	}
	glGenTextures(1, &skyboxTex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

	GLenum type = channels[0] == 4 ? GL_RGBA : GL_RGB;

	for (int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width[i], height[i], 0, type, GL_UNSIGNED_BYTE, texData[i]);
	}

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}


void GameTechRenderer::RenderFrame() {
	glEnable(GL_CULL_FACE);
	glClearColor(1, 1, 1, 1);
	BuildObjectList();
	SortObjectList();


	RenderShadowMap();// 渲染阴影


	RenderSkybox();
	RenderCamera();

	glDisable(GL_CULL_FACE); //Todo - text indices are going the wrong way...
}

void GameTechRenderer::InitLight()
{

	float pointLightPositions[3] = { 0.0, 10.0, -30.0 };
	float pointLightPositions02[3] = { 0.0, 10.0, 30.0 };
	float pointLightPositions03[3] = { 30.0, 10.0, 0.0 };
	float pointLightPositions04[3] = { -30.0, 10.0, 0.0 };

	float pointAmbient[3] = { 0.5 * 10, 0.5 * 10,0.5 * 10 };
	float pointDiffuse[3] = { 0.8 * 10, 0.8 * 10, 0.8 * 10 };
	float pointSpecular[3] = { 0.4 * 10, 0.4 * 10, 0.4 * 10 };

	float dirLightPos[] = { -0.2f, -1.0f, -0.3f };
	float dirAmbient[] = { 0.05f, 0.05f, 0.05f };
	float dirDiffuse[] = { 0.4f, 0.4f, 0.4f };
	float dirSpecular[] = { 0.5f, 0.5f, 0.5f };

	float spotLightPositions[3] = { 10.0, 30.0, 0.0 };
	float spotLightDirection[3] = { 1.0, -1.0, 1.0 };
	float spotAmbient[] = { 1.0f * 10.0f, 1.0f * 10.0f, 1.0f * 10.0f };
	float spotDiffuse[] = { 1.0f * 10.0f, 1.0f * 10.0f, 1.0f * 10.0f };
	float spotSpecular[] = { 1.0f * 10.0f, 1.0f * 10.0f, 1.0f * 10.0f };

	glUseProgram(shader->GetProgramID());

	// 控制是使用阴影还是光源
	glUniform1i(glGetUniformLocation(shader->GetProgramID(), "bshadermap"), (int)false);


	glUniform1i(glGetUniformLocation(shader->GetProgramID(), "material.diffuse"), 0);
	glUniform1i(glGetUniformLocation(shader->GetProgramID(), "material.specular"), 1);

	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "viewPos"), 1, &(gameWorld.GetMainCamera()->GetPosition())[0]);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "material.shininess"), 32.0f);

	// 设置(太阳)，为方便观察效果，已在shader中注释掉
	(glGetUniformLocation(shader->GetProgramID(), "dirLight.direction"), 1, &dirLightPos[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "dirLight.ambient"), 1, &dirAmbient[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "dirLight.diffuse"), 1, &dirDiffuse[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "dirLight.specular"), 1, &dirSpecular[0]);


	// 设置点光源
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[0].position"), 1, &pointLightPositions[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[0].ambient"), 1, &pointAmbient[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[0].diffuse"), 1, &pointDiffuse[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[0].specular"), 1, &pointSpecular[0]);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[0].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[0].linear"), 0.09f);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[0].quadratic"), 0.032f);


	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[1].position"), 1, &pointLightPositions02[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[1].ambient"), 1, &pointAmbient[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[1].diffuse"), 1, &pointDiffuse[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[1].specular"), 1, &pointSpecular[0]);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[1].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[1].linear"), 0.09f);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[1].quadratic"), 0.032f);

	
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[2].position"), 1, &pointLightPositions03[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[2].ambient"), 1, &pointAmbient[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[2].diffuse"), 1, &pointDiffuse[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[2].specular"), 1, &pointSpecular[0]);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[2].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[2].linear"), 0.09f);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[2].quadratic"), 0.032f);

	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[3].position"), 1, &pointLightPositions04[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[3].ambient"), 1, &pointAmbient[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[3].diffuse"), 1, &pointDiffuse[0]);
	glUniform3fv(glGetUniformLocation(shader->GetProgramID(), "pointLights[3].specular"), 1, &pointSpecular[0]);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[3].constant"), 1.0f);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[3].linear"), 0.09f);
	glUniform1f(glGetUniformLocation(shader->GetProgramID(), "pointLights[3].quadratic"), 0.032f);

	
	glUniform3fv(glGetUniformLocation(shadowShader->GetProgramID(), "spotLight.position"), 1, &spotLightPositions[0]);
	glUniform3fv(glGetUniformLocation(shadowShader->GetProgramID(), "spotLight.direction"), 1, &spotLightDirection[0]);
	glUniform3fv(glGetUniformLocation(shadowShader->GetProgramID(), "spotLight.ambient"), 1, &spotAmbient[0]);
	glUniform3fv(glGetUniformLocation(shadowShader->GetProgramID(), "spotLight.diffuse"), 1, &spotDiffuse[0]);
	glUniform3fv(glGetUniformLocation(shadowShader->GetProgramID(), "spotLight.specular"), 1, &spotSpecular[0]);
	glUniform1f(glGetUniformLocation(shadowShader->GetProgramID(), "spotLight.constant"), 1.0f);
	glUniform1f(glGetUniformLocation(shadowShader->GetProgramID(), "spotLight.linear"), 0.09f);
	glUniform1f(glGetUniformLocation(shadowShader->GetProgramID(), "spotLight.quadratic"), 0.032f);
	glUniform1f(glGetUniformLocation(shadowShader->GetProgramID(), "spotLight.cutOff"), cos(Maths::DegreesToRadians(12.5)));
	glUniform1f(glGetUniformLocation(shadowShader->GetProgramID(), "spotLight.outerCutOff"), cos(Maths::DegreesToRadians(15.0)));
}




void GameTechRenderer::BuildObjectList() {
	activeObjects.clear();

	gameWorld.OperateOnContents(
		[&](GameObject* o) {
			if (o->IsActive()) {
				const RenderObject* g = o->GetRenderObject();
				if (g) {
					activeObjects.emplace_back(g);
				}
			}
		}
	);
}

void GameTechRenderer::SortObjectList() {
	//std::sort(activeObjects.begin())

}

void GameTechRenderer::RenderShadowMap() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);

	glCullFace(GL_FRONT);

	BindShader(shadowShader);
	int mvpLocation = glGetUniformLocation(shadowShader->GetProgramID(), "mvpMatrix");

	Matrix4 shadowViewMatrix = Matrix4::BuildViewMatrix(lightPosition, Vector3(0, 0, 0), Vector3(0, 1, 0));
	Matrix4 shadowProjMatrix = Matrix4::Perspective(100.0f, 500.0f, 1, 45.0f);

	Matrix4 mvMatrix = shadowProjMatrix * shadowViewMatrix;

	shadowMatrix = biasMatrix * mvMatrix; //we'll use this one later on

	for (const auto& i : activeObjects) {
		Matrix4 modelMatrix = (*i).GetTransform()->GetMatrix();
		Matrix4 mvpMatrix = mvMatrix * modelMatrix;
		glUniformMatrix4fv(mvpLocation, 1, false, (float*)&mvpMatrix);
		BindMesh((*i).GetMesh());
		int layerCount = (*i).GetMesh()->GetSubMeshCount();
		for (int i = 0; i < layerCount; ++i) {
			DrawBoundMesh(i);
		}
	}

	glViewport(0, 0, currentWidth, currentHeight);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glCullFace(GL_BACK);
}

void GameTechRenderer::RenderSkybox() {
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	float screenAspect = (float)currentWidth / (float)currentHeight;
	Matrix4 viewMatrix = gameWorld.GetMainCamera()->BuildViewMatrix();
	Matrix4 projMatrix = gameWorld.GetMainCamera()->BuildProjectionMatrix(screenAspect);

	BindShader(skyboxShader);

	int projLocation = glGetUniformLocation(skyboxShader->GetProgramID(), "projMatrix");
	int viewLocation = glGetUniformLocation(skyboxShader->GetProgramID(), "viewMatrix");
	int texLocation = glGetUniformLocation(skyboxShader->GetProgramID(), "cubeTex");

	glUniformMatrix4fv(projLocation, 1, false, (float*)&projMatrix);
	glUniformMatrix4fv(viewLocation, 1, false, (float*)&viewMatrix);

	glUniform1i(texLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

	BindMesh(skyboxMesh);
	DrawBoundMesh();

	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void GameTechRenderer::RenderCamera() {
	float screenAspect = (float)currentWidth / (float)currentHeight;
	Matrix4 viewMatrix = gameWorld.GetMainCamera()->BuildViewMatrix();
	Matrix4 projMatrix = gameWorld.GetMainCamera()->BuildProjectionMatrix(screenAspect);

	OGLShader* activeShader = nullptr;
	int projLocation = 0;
	int viewLocation = 0;
	int modelLocation = 0;
	int colourLocation = 0;
	int hasVColLocation = 0;
	int hasTexLocation = 0;
	int shadowLocation = 0;

	int lightPosLocation = 0;
	int lightColourLocation = 0;
	int lightRadiusLocation = 0;

	int cameraLocation = 0;

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	for (const auto& i : activeObjects) {
		OGLShader* shader = (OGLShader*)(*i).GetShader();
		GLuint ttt = shader->GetProgramID();         //test variable J Xie
		BindShader(shader);

		BindTextureToShader((OGLTexture*)(*i).GetDefaultTexture(), "mainTex", 0);

		if (activeShader != shader) {
			projLocation = glGetUniformLocation(shader->GetProgramID(), "projMatrix");
			viewLocation = glGetUniformLocation(shader->GetProgramID(), "viewMatrix");
			modelLocation = glGetUniformLocation(shader->GetProgramID(), "modelMatrix");
			shadowLocation = glGetUniformLocation(shader->GetProgramID(), "shadowMatrix");
			colourLocation = glGetUniformLocation(shader->GetProgramID(), "objectColour");
			hasVColLocation = glGetUniformLocation(shader->GetProgramID(), "hasVertexColours");
			hasTexLocation = glGetUniformLocation(shader->GetProgramID(), "hasTexture");

			lightPosLocation = glGetUniformLocation(shader->GetProgramID(), "lightPos");
			lightColourLocation = glGetUniformLocation(shader->GetProgramID(), "lightColour");
			lightRadiusLocation = glGetUniformLocation(shader->GetProgramID(), "lightRadius");

			cameraLocation = glGetUniformLocation(shader->GetProgramID(), "cameraPos");
			glUniform3fv(cameraLocation, 1, (float*)&gameWorld.GetMainCamera()->GetPosition());

			glUniformMatrix4fv(projLocation, 1, false, (float*)&projMatrix);
			glUniformMatrix4fv(viewLocation, 1, false, (float*)&viewMatrix);

			glUniform3fv(lightPosLocation, 1, (float*)&lightPosition);
			glUniform4fv(lightColourLocation, 1, (float*)&lightColour);
			glUniform1f(lightRadiusLocation, lightRadius);

			int shadowTexLocation = glGetUniformLocation(shader->GetProgramID(), "shadowTex");
			glUniform1i(shadowTexLocation, 1);

			activeShader = shader;
		}

		InitLight();


		Matrix4 modelMatrix = (*i).GetTransform()->GetMatrix();
		glUniformMatrix4fv(modelLocation, 1, false, (float*)&modelMatrix);

		Matrix4 fullShadowMat = shadowMatrix * modelMatrix;
		glUniformMatrix4fv(shadowLocation, 1, false, (float*)&fullShadowMat);

		Vector4 colour = i->GetColour();
		glUniform4fv(colourLocation, 1, (float*)&colour);

		glUniform1i(hasVColLocation, !(*i).GetMesh()->GetColourData().empty());

		glUniform1i(hasTexLocation, (OGLTexture*)(*i).GetDefaultTexture() ? 1 : 0);

		BindMesh((*i).GetMesh());
		BindMaterial((*i).GetMaterial());
		int layerCount = (*i).GetMesh()->GetSubMeshCount();
		for (int i = 0; i < layerCount; ++i) {
			UpdateBoundMaterialLayer(i);
			DrawBoundMesh(i);
		}
	}
}

Matrix4 GameTechRenderer::SetupDebugLineMatrix()	const {
	float screenAspect = (float)currentWidth / (float)currentHeight;
	Matrix4 viewMatrix = gameWorld.GetMainCamera()->BuildViewMatrix();
	Matrix4 projMatrix = gameWorld.GetMainCamera()->BuildProjectionMatrix(screenAspect);

	return projMatrix * viewMatrix;
}

Matrix4 GameTechRenderer::SetupDebugStringMatrix()	const {
	return Matrix4::Orthographic(-1, 1.0f, 100, 0, 0, 100);
}
