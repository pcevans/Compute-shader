#pragma once
#include "controller.h"
//////////////////////////////////////
class RenderTextureClass
	{
	private:
		ID3D11Texture1D*			m_renderTargetTexture1D;
		ID3D11Texture2D*			m_renderTargetTexture2D;
		ID3D11Texture3D*			m_renderTargetTexture3D;
		ID3D11RenderTargetView*		m_renderTargetView;
		ID3D11ShaderResourceView*	m_shaderResourceView;
		ID3D11DepthStencilView*		m_DepthStencilView;
		ID3D11UnorderedAccessView*  m_pUAVs;
		bool uav;
		
	public:
	ID3D11Texture1D* GetTexture1D() { return m_renderTargetTexture1D; }
		int w, h;
		RenderTextureClass()
			{
			uav = FALSE;
			m_renderTargetTexture3D = NULL;
			m_renderTargetTexture2D = NULL;
			m_renderTargetTexture1D = NULL;
			m_renderTargetView = NULL;
			m_shaderResourceView = NULL;
			m_DepthStencilView = NULL;
			m_pUAVs = NULL;
			w = h = 0;
			}
		RenderTextureClass(const RenderTextureClass&) {}
		~RenderTextureClass() { Shutdown(); }
		ID3D11RenderTargetView* GetRenderTarget() { return m_renderTargetView; }
		//ID3D11Texture2D* GetRenderTargetTexture2D() { return m_renderTargetTexture2D; }
		
		bool Initialize_1DTex(ID3D11Device * device, HWND hwnd, int width = -1, bool uav_ = FALSE, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT, bool mipmaps = TRUE);
		bool Initialize_2DTex(ID3D11Device* device, HWND hwnd, int width = -1, int height = -1, bool uav_ = FALSE, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT, bool mipmaps = FALSE);
		bool Initialize_3DTex(ID3D11Device* device, int width, int height, int depth, bool uav_ = FALSE, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT, bool mipmaps = FALSE);
		
		bool InitializeStaging(ID3D11Device* device, HWND hwnd, int width = -1, int height = -1, bool uav_ = FALSE, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT, bool mipmaps = FALSE);
		bool Initialize_depth(ID3D11Device* device, HWND hwnd, int width = -1, int height = -1);
		void Shutdown();
		ID3D11DepthStencilView* GetDepthStencilView() { return m_DepthStencilView; }
		ID3D11ShaderResourceView* GetShaderResourceView();
		ID3D11UnorderedAccessView*  GetUAV() { return m_pUAVs; }

	};