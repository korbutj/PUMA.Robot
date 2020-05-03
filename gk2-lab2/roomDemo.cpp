#include "roomDemo.h"
#include <array>
#include "mesh.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;


RoomDemo::RoomDemo(HINSTANCE appInstance)
	: DxApplication(appInstance, 1280, 720, L"Pokój"), 
	//Constant Buffers
	m_cbWorldMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbProjMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbViewMtx(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()),
	m_cbSurfaceColor(m_device.CreateConstantBuffer<XMFLOAT4>()),
	m_cbLightPos(m_device.CreateConstantBuffer<XMFLOAT4>()),
	m_cbMapMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	//Textures
	m_smokeTexture(m_device.CreateShaderResourceView(L"resources/textures/smoke.png")),
	m_opacityTexture(m_device.CreateShaderResourceView(L"resources/textures/smokecolors.png")),
	m_lightMap(m_device.CreateShaderResourceView(L"resources/textures/light_cookie.png")),
	//Particles
	m_particles{ {-1.3f, -0.6f, -0.14f} }
{
	//Projection matrix
	auto s = m_window.getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;
	XMStoreFloat4x4(&m_projMtx, XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f));
	UpdateBuffer(m_cbProjMtx, m_projMtx);
	UpdateCameraCB();

	// TODO : 1.02 Calculate light projection matrix;
	XMStoreFloat4x4(&m_lightProjMtx, XMMatrixPerspectiveFovLH(LIGHT_FOV_ANGLE, 1.0f, LIGHT_NEAR, LIGHT_FAR));
	//Sampler States
	SamplerDescription sd;

	// TODO : 1.05 Create sampler with appropriate border color and addressing (border) and filtering (bilinear) modes
	sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.BorderColor[0] = 0.0f;
	sd.BorderColor[1] = 0.0f;
	sd.BorderColor[2] = 0.0f;
	sd.BorderColor[3] = 0.0f;
	sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	m_sampler = m_device.CreateSamplerState(sd);

	//Textures
	// TODO : 1.10 Create shadow texture with appropriate width, height, format, mip levels and bind flags
	Texture2DDescription td;
	td.Width = MAP_SIZE;
	td.Height = MAP_SIZE;
	td.Format = DXGI_FORMAT_R32_TYPELESS;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	td.MipLevels = 1;
	
	auto shadowTexture = m_device.CreateTexture(td);
	
	DepthStencilViewDescription dvd;

	// TODO : 1.11 Create depeth-stencil-view for the shadow texture with appropriate format
	dvd.Format = DXGI_FORMAT_D32_FLOAT;
	m_shadowDepthBuffer = m_device.CreateDepthStencilView(shadowTexture, dvd);

	ShaderResourceViewDescription srvd;

	// TODO : 1.12 Create shader resource view for the shadow texture with appropriate format, view dimensions, mip levels and most detailed mip level
	srvd.Format = DXGI_FORMAT_R32_FLOAT;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvd.Texture2D.MipLevels = 1;
	srvd.Texture2D.MostDetailedMip = 0;

	m_shadowMap = m_device.CreateShaderResourceView(shadowTexture, srvd);

	//Meshes
	vector<VertexPositionNormal> vertices;
	vector<unsigned short> indices;
	m_wall = Mesh::Rectangle(m_device, 4.0f);
	m_desk = Mesh::DoubleRect(m_device, 2.0f);
	m_weld = Mesh::Rectangle(m_device, 0.1f);
	m_box = Mesh::ShadedBox(m_device);

	m_puma[0] = Mesh::LoadMesh(m_device, L"resources/meshes/mesh1.mesh");
	m_puma[1] = Mesh::LoadMesh(m_device, L"resources/meshes/mesh2.mesh");
	m_puma[2] = Mesh::LoadMesh(m_device, L"resources/meshes/mesh3.mesh");
	m_puma[3] = Mesh::LoadMesh(m_device, L"resources/meshes/mesh4.mesh");
	m_puma[4] = Mesh::LoadMesh(m_device, L"resources/meshes/mesh5.mesh");
	m_puma[5] = Mesh::LoadMesh(m_device, L"resources/meshes/mesh6.mesh");

	//Init angles for puma
	for (int i = 0; i < 6;i++)
		a[i] = 0.0f;
	A[0] = { 0.0f,0.0f,0.0f,0.0f };
	A[1] = { 0.0f,0.27f,0.0f,0.0f };
	A[2] = { -0.91f,0.27f,0.0f,0.0f };
	A[3] = { 0.0f,0.27f,-0.26f ,0.0f };
	A[4] = { -1.72f,0.27f,0.0f,0.0f };
	//Init weld position
	weldPos = { 0.0f, 0.0f, 0.0f };


	m_vbParticles = m_device.CreateVertexBuffer<ParticleVertex>(ParticleSystem::MAX_PARTICLES);

	//World matrix of all objects
	auto translationToTheLeft = XMMatrixTranslation(0.0f, 0.0f, 2.0f);
	auto scale= XMMatrixScaling(2, 2, 2);
	auto a = 0.f;
	for (auto i = 0U; i < 4U; ++i, a += XM_PIDIV2)
		XMStoreFloat4x4(&m_wallsMtx[i], translationToTheLeft * XMMatrixRotationY(a)*scale);
	XMStoreFloat4x4(&m_wallsMtx[4], translationToTheLeft * XMMatrixRotationX(XM_PIDIV2) * scale);
	XMStoreFloat4x4(&m_wallsMtx[5], translationToTheLeft * XMMatrixRotationX(-XM_PIDIV2) * scale);

	translationToTheLeft = XMMatrixTranslation(-1.5f, 0.0f, 0.0f);


	XMStoreFloat4x4(&m_deskMtx, XMMatrixRotationX(XM_PI / 6) * XMMatrixRotationY(-XM_PIDIV2) * translationToTheLeft);
	XMStoreFloat4x4(&m_weldMtx, XMMatrixRotationX(XM_PI / 6) * XMMatrixRotationY(-XM_PIDIV2) * translationToTheLeft);
	XMStoreFloat4x4(&m_boxMtx, XMMatrixTranslation(-1.4f, -1.46f, -0.6f));
	
	XMStoreFloat3(&weldPos, XMVector3Transform(XMLoadFloat3(&weldPos), XMLoadFloat4x4(&m_weldMtx)));

	
	deskNormal = { 1, XM_PI / 6, 0 };
	deskA = { 0, 0, 1 };
	auto deskNormalVec = XMVector3Normalize(XMLoadFloat3(&deskNormal));
	XMStoreFloat3(&deskNormal, deskNormalVec);
	auto deskAVec = XMVector3Normalize(XMLoadFloat3(&deskA));
	XMStoreFloat3(&deskA, deskAVec);


	auto deskBVec = XMVector3Cross(deskNormalVec, deskAVec);
	XMStoreFloat3(&deskB, XMVector3Normalize(deskBVec));
	
	
	
	//create matrices for puma
	for (int i = 0; i < 6; i++)
	{
		XMStoreFloat4x4(&m_pumaMtx[i], XMMatrixTranslation(0.0f, 0.0f, 0.0f));
	}


	//Constant buffers content
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f });

	//Render states
	RasterizerDescription rsDesc;
	rsDesc.CullMode = D3D11_CULL_NONE;
	m_rsCullNone = m_device.CreateRasterizerState(rsDesc);

	m_bsAlpha = m_device.CreateBlendState(BlendDescription::AlphaBlendDescription());
	DepthStencilDescription dssDesc;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_dssNoWrite = m_device.CreateDepthStencilState(dssDesc);

	auto vsCode = m_device.LoadByteCode(L"phongVS.cso");
	auto psCode = m_device.LoadByteCode(L"phongPS.cso");
	m_phongVS = m_device.CreateVertexShader(vsCode);
	m_phongPS = m_device.CreatePixelShader(psCode);
	auto psRectCode = m_device.LoadByteCode(L"phongRectPS.cso");
	m_phongRectPS = m_device.CreatePixelShader(psRectCode);
	m_inputlayout = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);

	psCode = m_device.LoadByteCode(L"lightAndShadowPS.cso");
	m_lightShadowPS = m_device.CreatePixelShader(psCode);

	vsCode = m_device.LoadByteCode(L"particleVS.cso");
	psCode = m_device.LoadByteCode(L"particlePS.cso");
	auto gsCode = m_device.LoadByteCode(L"particleGS.cso");
	m_particleVS = m_device.CreateVertexShader(vsCode);
	m_particlePS = m_device.CreatePixelShader(psCode);
	m_particleGS = m_device.CreateGeometryShader(gsCode);
	m_particleLayout = m_device.CreateInputLayout<ParticleVertex>(vsCode);

	m_device.context()->IASetInputLayout(m_inputlayout.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	UpdateLamp(0.0f);

	//We have to make sure all shaders use constant buffers in the same slots!
	//Not all slots will be use by each shader
	ID3D11Buffer* vsb[] = { m_cbWorldMtx.get(),  m_cbViewMtx.get(), m_cbProjMtx.get() };
	m_device.context()->VSSetConstantBuffers(0, 3, vsb); //Vertex Shaders - 0: worldMtx, 1: viewMtx,invViewMtx, 2: projMtx, 3: tex1Mtx, 4: tex2Mtx
	m_device.context()->GSSetConstantBuffers(0, 1, vsb + 2); //Geometry Shaders - 0: projMtx
	ID3D11Buffer* psb[] = { m_cbSurfaceColor.get(), m_cbLightPos.get(), m_cbMapMtx.get() };
	m_device.context()->PSSetConstantBuffers(0, 3, psb); //Pixel Shaders - 0: surfaceColor, 1: lightPos, 2: mapMtx
}

void RoomDemo::UpdateCameraCB(XMMATRIX viewMtx)
{
	XMVECTOR det;
	XMMATRIX invViewMtx = XMMatrixInverse(&det, viewMtx);
	XMFLOAT4X4 view[2];
	XMStoreFloat4x4(view, viewMtx);
	XMStoreFloat4x4(view + 1, invViewMtx);
	UpdateBuffer(m_cbViewMtx, view);
}

void RoomDemo::UpdateLamp(float dt)
{
	static auto time = 0.0f;
	time += dt;
	auto swing = 0.3f * XMScalarSin(XM_2PI*time / 8);
	auto rot = XM_2PI*time / 20;
	auto lamp = XMMatrixTranslation(0.0f, -0.4f, 0.0f) * XMMatrixRotationX(swing) * XMMatrixRotationY(rot) *
		XMMatrixTranslation(0.0f, 2.0f, 0.0f);

	XMStoreFloat4x4(&m_lampMtx, lamp);

	XMFLOAT4 lightPos{ 0.0f, -0.05f, 0.0f, 1.0f };
	XMFLOAT4 lightTarget{ 0.0f, -10.0f, 0.0f, 1.0f };
	XMFLOAT4 upDir{ 1.0f, 0.0f, 0.0f, 0.0f };

	XMFLOAT4X4 texMtx;

	// TODO : 1.04 Calculate new light position in world coordinates
	XMFLOAT3 wcLightPos;
	XMStoreFloat3(&wcLightPos, XMVector3TransformCoord(XMLoadFloat4(&lightPos), lamp));
	UpdateBuffer(m_cbLightPos, XMFLOAT4(wcLightPos.x, wcLightPos.y, wcLightPos.z, 1.0f));
	//UpdateBuffer(m_cbLightPos, lightPos);

	// TODO : 1.01 Calculate light's view and inverted view matrix
	XMFLOAT3 lightCamPos;
	XMFLOAT3 lightCamTarget;
	XMFLOAT3 lightCamUpDir;
	XMStoreFloat3(&lightCamPos, XMVector3TransformCoord(XMLoadFloat4(&lightPos), lamp));
	XMStoreFloat3(&lightCamTarget, XMVector3TransformCoord(XMLoadFloat4(&lightTarget), lamp));
	XMStoreFloat3(&lightCamUpDir, XMVector3TransformNormal(XMLoadFloat4(&upDir), lamp));
	auto lightCamViewMtx = XMMatrixLookAtLH(XMLoadFloat3(&lightCamPos), XMLoadFloat3(&lightCamTarget), XMLoadFloat3(&lightCamUpDir));
	XMStoreFloat4x4(&m_lightViewMtx[0], lightCamViewMtx);
	XMStoreFloat4x4(&m_lightViewMtx[1], XMMatrixInverse(nullptr, lightCamViewMtx));

	// TODO : 1.03 Calculate map transform matrix
	auto mapMtx = XMLoadFloat4x4(&m_lightViewMtx[0]) * 
				  XMLoadFloat4x4(&m_lightProjMtx) * 
				  XMMatrixScaling(0.5f, -0.5f, 1.0f) * 
				  XMMatrixTranslation(0.5f, 0.5f, 0.0f);

	// TODO : 1.19 Modify map transform to fix z-fighting
	mapMtx = mapMtx * XMMatrixTranslation(0.0f, 0.0f, -0.00001f);
	XMStoreFloat4x4(&texMtx, mapMtx);

	UpdateBuffer(m_cbMapMtx, texMtx);

}

void mini::gk2::RoomDemo::UpdateParticles(float dt)
{
	auto verts = m_particles.Update(dt, m_camera.getCameraPosition());
	UpdateBuffer(m_vbParticles, verts);
}

void mini::gk2::RoomDemo::UpdateWeld(float dt)
{
	static auto time = 0.0f;
	time += dt;
	float r = 0.6f;
	weldPos.x = -1.5f + (r * XMScalarCos(time) * deskA.x) + (r * XMScalarSin(time) * deskB.x);
	weldPos.y = (r * XMScalarCos(time) * deskA.y) + (r * XMScalarSin(time) * deskB.y);
	weldPos.z = (r * XMScalarCos(time) * deskA.z) + (r * XMScalarSin(time) * deskB.z);

	XMVECTOR vec = XMLoadFloat3(&weldPos);
	//vec=XMVector3Transform(vec, temp);
	XMStoreFloat3(&weldPos, vec);

	XMStoreFloat4x4(&m_weldMtx, XMMatrixTranslation(weldPos.x, weldPos.y, weldPos.z));

}

void mini::gk2::RoomDemo::UpdatePuma()
{
	inverse_kinematics(weldPos, deskNormal, a[0], a[1], a[2], a[3], a[4]);
	UpdatePumaMatrices();
}

void mini::gk2::RoomDemo::UpdatePumaMatrices()
{
	XMMATRIX mat[5];
	mat[0] = XMMatrixRotationY(a[0]);
	mat[1] = XMMatrixTranslation(0.0f, -0.27f, 0.0f) * XMMatrixRotationZ(a[1]) * XMMatrixTranslation(0.0f, 0.27f, 0.0f) * mat[0];
	mat[2] = XMMatrixTranslation(0.91f, -0.27f, 0.0f) * XMMatrixRotationZ(a[2]) * XMMatrixTranslation(-0.91f, 0.27f, 0.0f) * mat[1];
	mat[3] = XMMatrixTranslation(0.0f, -0.27f, 0.26f) * XMMatrixRotationX(a[3]) * XMMatrixTranslation(0.0f, 0.27f, -0.26f) * mat[2];
	mat[4] = XMMatrixTranslation(1.72f, -0.27f, 0.0f) * XMMatrixRotationZ(a[4]) * XMMatrixTranslation(-1.72f, 0.27f, 0.0f) * mat[3];

	for (int i = 0; i < 5; i++)
	{
		XMStoreFloat4x4(&m_pumaMtx[i+1], mat[i]);
	}	
	
}
void mini::gk2::RoomDemo::inverse_kinematics(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 normal, float& a1, float& a2, float& a3, float& a4, float& a5)
{
	float l1 = .91f, l2 = .81f, l3 = .33f, dy = .27f, dz = .26f;
	XMVECTOR temp = XMVector3Normalize(XMLoadFloat3(&normal));
	XMFLOAT3 Normal;
	XMStoreFloat3(&Normal, temp);
	XMFLOAT3 pos1;
	pos1.x = pos.x + Normal.x * l3;
	pos1.y = pos.y + Normal.y * l3;
	pos1.z = pos.z + Normal.z * l3;
	float e = sqrtf(pos1.z * pos1.z + pos1.x * pos1.x - dz * dz);
	a1 = atan2(pos1.z, -pos1.x) + atan2(dz, e);
	XMFLOAT3 pos2(e, pos1.y - dy, .0f);
	a3 = -acosf(min(1.0f, (pos2.x * pos2.x + pos2.y * pos2.y - l1 * l1 - l2 * l2) / (2.0f * l1 * l2)));
	float k = l1 + l2 * cosf(a3), l = l2 * sinf(a3);
	a2 = -atan2(pos2.y, sqrtf(pos2.x * pos2.x + pos2.z * pos2.z)) - atan2(l, k);

	
	XMVECTOR normal1=XMVector3Transform(temp, XMMatrixRotationY(-a1));
	normal1 = XMVector3Transform(normal1, XMMatrixRotationZ(-(a2+a3)));
	XMFLOAT3 res;
	XMStoreFloat3(&res, normal1);
	a5 = acosf(res.x);
	a4 = atan2(res.z, res.y);
}



void RoomDemo::Update(const Clock& c)
{
	double dt = c.getFrameTime();
	HandleCameraInput(dt);
	UpdateLamp(static_cast<float>(dt));
	UpdateParticles(dt);
	UpdateWeld(dt);
	UpdatePuma();
}

void RoomDemo::SetWorldMtx(DirectX::XMFLOAT4X4 mtx)
{
	UpdateBuffer(m_cbWorldMtx, mtx);
}

void mini::gk2::RoomDemo::SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps)
{
	m_device.context()->VSSetShader(vs.get(), nullptr, 0);
	m_device.context()->PSSetShader(ps.get(), nullptr, 0);
}

void mini::gk2::RoomDemo::SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler)
{
	m_device.context()->PSSetShaderResources(0, resList.size(), resList.begin());
	auto s_ptr = sampler.get();
	m_device.context()->PSSetSamplers(0, 1, &s_ptr);
}

void RoomDemo::DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx)
{
	SetWorldMtx(worldMtx);
	m.Render(m_device.context());
}

void RoomDemo::DrawParticles()
{
	//Set input layout, primitive topology, shaders, vertex buffer, and draw particles
	SetTextures({ m_smokeTexture.get(), m_opacityTexture.get() });
	m_device.context()->IASetInputLayout(m_particleLayout.get());
	SetShaders(m_particleVS, m_particlePS);
	m_device.context()->GSSetShader(m_particleGS.get(), nullptr, 0);
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	unsigned int stride = sizeof(ParticleVertex);
	unsigned int offset = 0;
	auto vb = m_vbParticles.get();
	m_device.context()->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	m_device.context()->Draw(m_particles.particlesCount(), 0);

	//Reset layout, primitive topology and geometry shader
	m_device.context()->GSSetShader(nullptr, nullptr, 0);
	m_device.context()->IASetInputLayout(m_inputlayout.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void mini::gk2::RoomDemo::DrawPuma()
{
	for (int i = 0; i < 6; i++)
	{
		DrawMesh(m_puma[i], m_pumaMtx[i]);
	}
}

void RoomDemo::DrawScene()
{
	for (auto& wallMtx : m_wallsMtx)
		DrawMesh(m_wall, wallMtx);

	SetShaders(m_phongVS, m_phongRectPS);
	DrawMesh(m_desk, m_deskMtx);

	SetShaders(m_phongVS, m_phongPS);
	DrawMesh(m_weld, m_weldMtx);
	DrawPuma();
	

	//Draw screen
	m_device.context()->RSSetState(m_rsCullNone.get());

	
	m_device.context()->RSSetState(nullptr);
}

void RoomDemo::Render()
{
	Base::Render();

	

	// TODO : 1.13 Copy light's view/inverted view and projection matrix to constant buffers
	UpdateBuffer(m_cbViewMtx, m_lightViewMtx);
	UpdateBuffer(m_cbProjMtx, m_lightProjMtx);

	// TODO : 1.14 Set up view port of the appropriate size
	Viewport vp = Viewport({ MAP_SIZE, MAP_SIZE });
	m_device.context()->RSSetViewports(1, &vp);

	// TODO : 1.15 Bind no render targets and the shadow map as depth buffer
	m_device.context()->OMSetRenderTargets(0, 0, m_shadowDepthBuffer.get());
	// TODO : 1.16 Clear the depth buffer
	m_device.context()->ClearDepthStencilView(m_shadowDepthBuffer.get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	// TODO : 1.17 Render objects and particles (w/o blending) to the shadow map using Phong shaders
	SetShaders(m_phongVS, m_phongPS);
	DrawScene();
	DrawParticles();

	ResetRenderTarget();
	UpdateBuffer(m_cbProjMtx, m_projMtx);
	UpdateCameraCB();

	// TODO : 1.08 Bind m_lightMap and m_shadowMap textures then draw objects and particles using light&shadow pixel shader
	SetTextures({ m_lightMap.get(), m_shadowMap.get() }, m_sampler);
	SetShaders(m_phongVS, m_lightShadowPS);

	DrawScene();

	m_device.context()->OMSetBlendState(m_bsAlpha.get(), nullptr, UINT_MAX);
	m_device.context()->OMSetDepthStencilState(m_dssNoWrite.get(), 0);
	DrawParticles();
	m_device.context()->OMSetBlendState(nullptr, nullptr, UINT_MAX);
	m_device.context()->OMSetDepthStencilState(nullptr, 0);
}