#pragma once
#include "dxApplication.h"
#include "mesh.h"
#include "particleSystem.h"

namespace mini::gk2
{
	class RoomDemo : public DxApplication
	{
	public:
		using Base = DxApplication;

		explicit RoomDemo(HINSTANCE appInstance);

	protected:
		void Update(const Clock& dt) override;
		void Render() override;

	private:
#pragma region CONSTANTS
		
		static constexpr unsigned int MAP_SIZE = 1024;
		static constexpr float LIGHT_NEAR = 0.35f;
		static constexpr float LIGHT_FAR = 5.5f;
		static constexpr float LIGHT_FOV_ANGLE = DirectX::XM_PI / 3.0f;



#pragma endregion


		dx_ptr<ID3D11Buffer> m_cbWorldMtx, //vertex shader constant buffer slot 0
			m_cbProjMtx;	//vertex shader constant buffer slot 2 & geometry shader constant buffer slot 0
		dx_ptr<ID3D11Buffer> m_cbViewMtx; //vertex shader constant buffer slot 1
		dx_ptr<ID3D11Buffer> m_cbSurfaceColor;	//pixel shader constant buffer slot 0
		dx_ptr<ID3D11Buffer> m_cbLightPos; //pixel shader constant buffer slot 1
		dx_ptr<ID3D11Buffer> m_cbMapMtx; //pixel shader constant buffer slot 2

		Mesh m_wall; //uses m_wallsMtx[6]
		
		Mesh m_box; //uses m_boxMtx
		Mesh m_lamp; //uses m_lampMtx
		
		Mesh m_puma[6];
		Mesh m_desk;
		Mesh m_weld;

		//angles for all parts of puma
		float a[6];
		DirectX::XMFLOAT4 A[6];
		DirectX::XMFLOAT3 weldPos;
		DirectX::XMFLOAT3 deskNormal;
		DirectX::XMFLOAT3 deskA;
		DirectX::XMFLOAT3 deskB;
		dx_ptr<ID3D11Buffer> m_vbParticles;

		DirectX::XMFLOAT4X4 m_projMtx, m_wallsMtx[6], m_boxMtx, m_lampMtx, m_lightViewMtx[2], m_lightProjMtx, m_deskMtx, m_weldMtx;
		DirectX::XMFLOAT4X4 m_pumaMtx[6];

		dx_ptr<ID3D11SamplerState> m_sampler;

		dx_ptr<ID3D11ShaderResourceView> m_smokeTexture, m_opacityTexture, m_lightMap, m_shadowMap;

		dx_ptr<ID3D11RenderTargetView> m_shadowRenderTarget;
		dx_ptr<ID3D11DepthStencilView> m_shadowDepthBuffer;

		dx_ptr<ID3D11RasterizerState> m_rsCullNone;
		dx_ptr<ID3D11BlendState> m_bsAlpha;
		dx_ptr<ID3D11DepthStencilState> m_dssNoWrite;

		dx_ptr<ID3D11InputLayout> m_inputlayout, m_particleLayout;

		dx_ptr<ID3D11VertexShader> m_phongVS, m_particleVS;
		dx_ptr<ID3D11GeometryShader> m_particleGS;
		dx_ptr<ID3D11PixelShader> m_phongPS, m_lightShadowPS, m_particlePS;

		ParticleSystem m_particles;

		void UpdateCameraCB(DirectX::XMMATRIX viewMtx);
		void UpdateCameraCB() { UpdateCameraCB(m_camera.getViewMatrix()); }
		void UpdateLamp(float dt);
		void UpdateParticles(float dt);
		void UpdatePumaMatrices();
		void UpdateWeld(float dt);
		void UpdatePuma();
		void inverse_kinematics(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 normal, float& a1, float& a2, float& a3, float& a4, float& a5);

		void DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx);
		void DrawParticles();

		void SetWorldMtx(DirectX::XMFLOAT4X4 mtx);
		void SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps);
		void SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler);
		void SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList) { SetTextures(std::move(resList), m_sampler); }
		
		void DrawScene();
		void DrawPuma();

	};
}