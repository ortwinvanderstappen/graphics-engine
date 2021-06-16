#include "stdafx.h"
#include "ParticleEmitterComponent.h"
#include <utility>
#include "EffectHelper.h"
#include "ContentManager.h"
#include "TextureDataLoader.h"
#include "Particle.h"
#include "TransformComponent.h"
#include <algorithm>

ParticleEmitterComponent::ParticleEmitterComponent(std::wstring assetFile, int particleCount) :
	m_pVertexBuffer(nullptr),
	m_pEffect(nullptr),
	m_pParticleTexture(nullptr),
	m_pInputLayout(nullptr),
	m_pInputLayoutSize(0),
	m_Settings(ParticleEmitterSettings()),
	m_ParticleCount(particleCount),
	m_ActiveParticles(0),
	m_LastParticleInit(0.0f),
	m_AssetFile(std::move(assetFile)),
	m_IsActive(true)
{
	// Initialize the particle pool
	for (int i = 0; i < m_ParticleCount; ++i)
	{
		m_Particles.push_back(new Particle(m_Settings));
	}
}

ParticleEmitterComponent::~ParticleEmitterComponent()
{
	// Delete all the particles
	for (Particle* pParticle : m_Particles)
	{
		delete pParticle;
		pParticle = nullptr;
	}
	m_Particles.clear();

	// Release GPU resources
	m_pInputLayout->Release();
	m_pVertexBuffer->Release();
}

void ParticleEmitterComponent::Initialize(const GameContext& gameContext)
{
	LoadEffect(gameContext);
	CreateVertexBuffer(gameContext);
	m_pParticleTexture = ContentManager::Load<TextureData>(m_AssetFile);
}

void ParticleEmitterComponent::LoadEffect(const GameContext& gameContext)
{
	// Load effect
	m_pEffect = ContentManager::Load<ID3DX11Effect>(L"./Resources/Effects/ParticleRenderer.fx");
	m_pDefaultTechnique = m_pEffect->GetTechniqueByIndex(0);

	// World view projection variable
	m_pWvpVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
	if (!m_pWvpVariable->IsValid())
		Logger::LogWarning(L"ParticleEmitterComponent: Invalid Shader Variable! (gWorldViewProj)");

	// View inverse variable
	m_pViewInverseVariable = m_pEffect->GetVariableByName("gViewInverse")->AsMatrix();
	if (!m_pViewInverseVariable->IsValid())
		Logger::LogWarning(L"ParticleEmitterComponent: Invalid Shader Variable! (gViewInverse)");

	// Particle texture variable
	m_pTextureVariable = m_pEffect->GetVariableByName("gParticleTexture")->AsShaderResource();
	if (!m_pTextureVariable->IsValid())
		Logger::LogWarning(L"ParticleEmitterComponent: Invalid Shader Variable! (gParticleTexture)");

	EffectHelper::BuildInputLayout(gameContext.pDevice, m_pDefaultTechnique, &m_pInputLayout, m_pInputLayoutSize);
}

void ParticleEmitterComponent::CreateVertexBuffer(const GameContext& gameContext)
{
	// If the vertex buffer exists, release it
	if (m_pVertexBuffer)
	{
		m_pVertexBuffer->Release();
	}

	// Create a dynamic vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(ParticleVertex) * m_ParticleCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;

	// Create the buffer
	const HRESULT result = gameContext.pDevice->CreateBuffer(&vertexBufferDesc, nullptr, &m_pVertexBuffer);
	if (Logger::LogHResult(result, L"ParticleEmitterComponent::CreateVertexBuffer() > Create buffer Failed!")) return;
}

void ParticleEmitterComponent::Update(const GameContext& gameContext)
{
	const float elapsedSeconds = gameContext.pGameTime->GetElapsed();

	// Calculate particle interval
	const float averageParticleEnergy = (m_Settings.MinEnergy + m_Settings.MaxEnergy) * 0.5f;
	const float particleInterval = averageParticleEnergy / static_cast<float>(m_ParticleCount);

	// Increase last spawn timer
	m_LastParticleInit += elapsedSeconds;

	// Particle validation and adding active ones to the vertexBuffer
	m_ActiveParticles = 0;

	//BUFFER MAPPING CODE 
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	// Map retrieves a subresource which contains the pointer to the first element in our vertexbuffer
	// We are able to change the data between Map and Unmap				// Discard previous data
	gameContext.pDeviceContext->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	ParticleVertex* pBuffer = static_cast<ParticleVertex*>(mappedResource.pData); // Retrieve the first element of the buffer

	for (Particle* pParticle : m_Particles)
	{
		pParticle->Update(gameContext);

		// Add all the current active particles
		if (pParticle->IsActive())
		{
			pBuffer[m_ActiveParticles] = pParticle->GetVertexInfo();
			++m_ActiveParticles;
		}
		// "Spawn" a new particle
		else if (m_IsActive && m_LastParticleInit >= particleInterval)
		{
			pParticle->Init(GetTransform()->GetWorldPosition());
			pBuffer[m_ActiveParticles] = pParticle->GetVertexInfo();
			++m_ActiveParticles;
			// Reset timer
			m_LastParticleInit -= particleInterval;
		}
	}
	gameContext.pDeviceContext->Unmap(m_pVertexBuffer, 0);
}

void ParticleEmitterComponent::Draw(const GameContext&)
{}

void ParticleEmitterComponent::PostDraw(const GameContext& gameContext)
{
	// Set world view projection variable (the particles are already translated to world on the cpu, so we only set the view projection)
	if (m_pWvpVariable)
	{
		m_pWvpVariable->SetMatrix(&gameContext.pCamera->GetViewProjection()._11);
	}
	// View inverse matrix
	if (m_pViewInverseVariable)
	{
		m_pViewInverseVariable->SetMatrix(&gameContext.pCamera->GetViewInverse()._11);
	}
	// Particle texture
	if (m_pParticleTexture && m_pTextureVariable)
	{
		m_pTextureVariable->SetResource(m_pParticleTexture->GetShaderResourceView());
	}

	auto* pDeviceContext = gameContext.pDeviceContext;
	// Set the Input Layout
	pDeviceContext->IASetInputLayout(m_pInputLayout);

	// Set the Primitive Topology
	pDeviceContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);

	// Set the Vertex Buffer
	unsigned int stride = sizeof(ParticleVertex);
	unsigned int offset = 0;
	pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// Technique passes
	D3DX11_TECHNIQUE_DESC techDesc;
	m_pDefaultTechnique->GetDesc(&techDesc);
	for (unsigned int i = 0; i < techDesc.Passes; ++i)
	{
		m_pDefaultTechnique->GetPassByIndex(i)->Apply(0, pDeviceContext);
		pDeviceContext->Draw(m_ActiveParticles, 0);
	}
}
