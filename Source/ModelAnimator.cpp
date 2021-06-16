#include "stdafx.h"
#include "ModelAnimator.h"

ModelAnimator::ModelAnimator(MeshFilter* pMeshFilter) :
	m_pMeshFilter(pMeshFilter),
	m_Transforms(std::vector<DirectX::XMFLOAT4X4>()),
	m_IsPlaying(false),
	m_Reversed(false),
	m_ClipSet(false),
	m_TickCount(0),
	m_AnimationSpeed(1.0f)
{
	SetAnimation(0);
}

void ModelAnimator::SetAnimation(UINT clipNumber)
{
	//Set m_ClipSet to false
	m_ClipSet = false;

	//Check if clipNumber is NOT smaller than the actual m_AnimationClips vector size
	if (!(clipNumber < GetClipCount()))
	{
		//	Call Reset
		Reset();
		//	Log a warning with an appropriate message
		Logger::LogWarning(L"ModelAnimator.cpp: Clipnumber exceeded the clipcount.");
		//	return
		return;
	}

	//	Retrieve the AnimationClip from the m_AnimationClips vector based on the given clipNumber
	//	Call SetAnimation(AnimationClip clip)
	SetAnimation(m_pMeshFilter->m_AnimationClips[clipNumber]);
}

void ModelAnimator::SetAnimation(std::wstring clipName)
{
	//Set m_ClipSet to false
	m_ClipSet = false;
	//Iterate the m_AnimationClips vector and search for an AnimationClip with the given name (clipName)

	AnimationClip* pClip = nullptr;
	for (AnimationClip& clip : m_pMeshFilter->m_AnimationClips)
	{
		if (clip.Name == clipName)
		{
			pClip = &clip;
			break;
		}
	}

	if (pClip == nullptr)
	{
		//	Call Reset
		//	Log a warning with an appropriate message
		Reset();
		const std::wstring msg = L"ModelAnimator.cpp: Clip with name " + clipName + L" was not found.";
		Logger::LogWarning(msg);
		return;
	}

	//If found,
	//	Call SetAnimation(Animation Clip) with the found clip
	SetAnimation(*pClip);
}

void ModelAnimator::SetAnimation(AnimationClip clip)
{
	//Set m_ClipSet to true
	m_ClipSet = true;
	//Set m_CurrentClip
	m_CurrentClip = clip;
	//Call Reset(false)
	Reset(false);
}

void ModelAnimator::Reset(bool pause)
{
	//If pause is true, set m_IsPlaying to false
	if (pause) m_IsPlaying = false;

	//Set m_TickCount to zero
	m_TickCount = 0;
	//Set m_AnimationSpeed to 1.0f
	m_AnimationSpeed = 1.f;

	//If m_ClipSet is true
	if (m_ClipSet)
	{
		//	Retrieve the BoneTransform from the first Key from the current clip (m_CurrentClip)
		const std::vector<DirectX::XMFLOAT4X4>& boneTransforms = m_CurrentClip.Keys[0].BoneTransforms;
		//	Refill the m_Transforms vector with the new BoneTransforms (have a look at vector::assign)
		m_Transforms.assign(boneTransforms.begin(), boneTransforms.end());
	}
	else
	{
		//	Create an IdentityMatrix
		const DirectX::XMMATRIX identityMatrix{ DirectX::XMMatrixIdentity() };
		//	Refill the m_Transforms vector with this IdenityMatrix (Amount = BoneCount) (have a look at vector::assign)
		DirectX::XMFLOAT4X4 identityMatrix4X4{};
		DirectX::XMStoreFloat4x4(&identityMatrix4X4, identityMatrix);
		m_Transforms.assign(m_pMeshFilter->m_BoneCount, identityMatrix4X4);
	}
}

void ModelAnimator::Update(const GameContext& gameContext)
{
	const float elapsedSeconds = gameContext.pGameTime->GetElapsed();

	//We only update the transforms if the animation is running and the clip is set
	if (m_IsPlaying && m_ClipSet)
	{
		//1. 
		//Calculate the passedTicks (see the lab document)
		float passedTicks = elapsedSeconds * m_CurrentClip.TicksPerSecond * m_AnimationSpeed;
		//Make sure that the passedTicks stay between the m_CurrentClip.Duration bounds (fmod)
		passedTicks = fmod(passedTicks, m_CurrentClip.Duration);

		//2. 
		//IF m_Reversed is true
		if (m_Reversed)
		{
			//	Subtract passedTicks from m_TickCount
			m_TickCount -= passedTicks;
			//	If m_TickCount is smaller than zero, add m_CurrentClip.Duration to m_TickCount
			if (m_TickCount < 0) m_TickCount += m_CurrentClip.Duration;
		}
		else
		{
			//	Add passedTicks to m_TickCount
			m_TickCount += passedTicks;
			//	if m_TickCount is bigger than the clip duration, subtract the duration from m_TickCount
			if (m_TickCount > m_CurrentClip.Duration) m_TickCount -= m_CurrentClip.Duration;
		}

		//3.
		//Find the enclosing keys
		AnimationKey keyA, keyB;
		AnimationKey* pClosestKeyA = nullptr, * pClosestKeyB = nullptr;
		//Iterate all the keys of the clip and find the following keys:
		for (AnimationKey& key : m_CurrentClip.Keys)
		{
			//keyA > Closest Key with Tick before/smaller than m_TickCount
			if (key.Tick < m_TickCount)
			{
				if (pClosestKeyA == nullptr) pClosestKeyA = &key;

				// Smaller than tickcount, but closer to tick than other key
				if (pClosestKeyA->Tick < key.Tick) pClosestKeyA = &key;
			}

			//keyB > Closest Key with Tick after/bigger than m_TickCount
			if (key.Tick > m_TickCount)
			{
				if (pClosestKeyB == nullptr) pClosestKeyB = &key;
				// Bigger than tickcount, but closer to tick than other key
				if (pClosestKeyB->Tick > key.Tick) pClosestKeyB = &key;
			}
		}
		if (pClosestKeyA == nullptr || pClosestKeyB == nullptr)
		{
			Logger::LogWarning(L"ModelAnimator.cpp: One of the enclosing keys was nullptr");
			return;
		}
		keyA = *pClosestKeyA;
		keyB = *pClosestKeyB;

		//4.
		//Interpolate between keys
		//Figure out the BlendFactor (See lab document)
		const float newTickCount = m_TickCount - keyA.Tick;
		const float maxTick = keyB.Tick - keyA.Tick;
		const float blendFactor = newTickCount / maxTick;

		//Clear the m_Transforms vector
		m_Transforms.clear();
		//FOR every boneTransform in a key (So for every bone)
		for (int boneIdx{ 0 }; boneIdx < m_pMeshFilter->m_BoneCount; ++boneIdx)
		{
			//Retrieve the transform from keyA (transformA)
			const DirectX::XMFLOAT4X4& transformA = keyA.BoneTransforms[boneIdx];
			// Retrieve the transform from keyB (transformB)
			const DirectX::XMFLOAT4X4& transformB = keyB.BoneTransforms[boneIdx];

			// Decompose both transforms
			// A
			DirectX::FXMMATRIX matrixA = DirectX::XMLoadFloat4x4(&transformA);
			DirectX::XMVECTOR scaleA{};
			DirectX::XMVECTOR rotA{};
			DirectX::XMVECTOR transA{};
			DirectX::XMMatrixDecompose(&scaleA, &rotA, &transA, matrixA);
			// B
			DirectX::FXMMATRIX matrixB = DirectX::XMLoadFloat4x4(&transformB);
			DirectX::XMVECTOR scaleB{};
			DirectX::XMVECTOR rotB{};
			DirectX::XMVECTOR transB{};
			DirectX::XMMatrixDecompose(&scaleB, &rotB, &transB, matrixB);

			using namespace DirectX;

			//	Lerp between all the transformations (Position, Scale, Rotation)
			DirectX::XMVECTOR scaleLerped{ XMVectorLerp(scaleA, scaleB, blendFactor) };
			DirectX::XMVECTOR rotLerped{ XMQuaternionSlerp(rotA, rotB, blendFactor) };
			DirectX::XMVECTOR transLerped{ XMVectorLerp(transA, transB, blendFactor) };
			
			//	Compose a transformation matrix with the lerp-results
			FXMMATRIX scaleMatrix = XMMatrixScalingFromVector(scaleLerped);
			FXMMATRIX rotMatrix = XMMatrixRotationQuaternion(rotLerped);
			FXMMATRIX transMatrix = XMMatrixTranslationFromVector(transLerped);
			XMMATRIX resultMatrix = scaleMatrix * rotMatrix * transMatrix;
			XMFLOAT4X4 resultMatrix4X4{};
			XMStoreFloat4x4(&resultMatrix4X4, resultMatrix);
			
			//	Add the resulting matrix to the m_Transforms vector
			m_Transforms.push_back(resultMatrix4X4);
		}
	}
}
