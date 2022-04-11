///***************************************************************************************
// Karl Zingel Assignment 2 - GAME3111
//***************************************************************************************

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include "Waves.h"
#include "../Common/Camera.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;


	//Bounding Box for Collision

	BoundingBox bounding_box;
	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Count
};

class CastleApp : public D3DApp
{
public:
	CastleApp(HINSTANCE hInstance);
	CastleApp(const CastleApp& rhs) = delete;
	CastleApp& operator=(const CastleApp& rhs) = delete;
	~CastleApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	void GetMovementBooleans(bool& forward, bool& backward, bool& left, bool& right);

	void OnKeyboardInput(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);

	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayouts();
	void BuildLandGeometry();
	void BuildWavesGeometry();
	void BuildGeometry();
	void BuildTreeSpritesGeometry();
	void BuildLightningSpritesGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void CheckCollision(std::vector<XMMATRIX> positions, XMMATRIX cameraPosition);
	void Build_Render_Item_Rotate(const char* item, XMMATRIX scale_matrix, XMMATRIX translate_matrix, XMMATRIX rotation_matrix,
	                              const char* material, UINT ObjIndex);
	void Build_Render_Item(const char* item, XMMATRIX scale_matrix, XMMATRIX translate_matrix,
		const char* material, UINT ObjIndex);
	void Build_Render_Item_Collision(const char* item, XMMATRIX scale_matrix, XMMATRIX translate_matrix,
	                                 const char* material, UINT ObjIndex);
	void BuildMaze(UINT& objCBIndex);
	void BuildCastle(UINT& objCBIndex);
	void Build_Render_Items();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	float GetHillsHeight(float x, float z)const;
	XMFLOAT3 GetHillsNormal(float x, float z)const;

private:

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mStdInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

	RenderItem* mWavesRitem = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[static_cast<int>(RenderLayer::Count)];

	std::unique_ptr<Waves> mWaves;

	PassConstants mMainPassCB;

	// Old Camera Code
	/*XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 50.0f;*/

	POINT mLastMousePos;

	//Define our camera
	Camera m_Camera;

	//Set camera speed manually
	float m_CameraSpeed = 12.5f;

	//Movement booleans ( modified in function ) 
	bool moveForward = true;
	bool moveBackward = true;
	bool moveLeft = true;
	bool moveRight = true;

	//Temp_Cam - temporary variable modified/returned in intersects function(s).
	float temp_cam = 0.0f;
	//Distance allowed between camera and bounding box.
	float camera_collision_distance = 3.0f;
	
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		CastleApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

CastleApp::CastleApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

CastleApp::~CastleApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool CastleApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mWaves = std::make_unique<Waves>(248, 248, 1.0f, 0.03f, 4.0f, 0.2f);
	m_Camera.SetPosition(0.0f, 18.5f, -110.0f);
	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayouts();
	BuildLandGeometry();
	BuildWavesGeometry();
	BuildGeometry();
	BuildTreeSpritesGeometry();
	BuildLightningSpritesGeometry();
	BuildMaterials();
	Build_Render_Items();
	BuildFrameResources();
	BuildPSOs();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void CastleApp::OnResize()
{
	D3DApp::OnResize();

	

	m_Camera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	//Old Camera Code
	 /*//The window resized, so update the aspect ratio and recompute the projection matrix.
	 XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	  XMStoreFloat4x4(&mProj, P);*/
}

void CastleApp::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
	UpdateWaves(gt);

	
}

void CastleApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::MediumPurple, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
	
	//4 PSOS
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

	mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void CastleApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void CastleApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CastleApp::OnMouseMove(WPARAM btnState, int x, int y)
{

	
	//if left mouse button is down and moving
	if((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		//step4: Instead of updating the angles based on input to orbit camera around scene, 
		//we rotate the camera’s look direction:
		//mTheta += dx;
		//mPhi += dy;

		m_Camera.Pitch(dy);
		m_Camera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;


	
	//Old Camera Code
	/*if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.2f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.2f * static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	}*/

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}
void CastleApp::GetMovementBooleans(bool& forward, bool& backward, bool& left, bool& right)
{
	
	//Reset movement booleans to true - modified in for loop and returned to movement function scope as reference.
	moveForward = true;
	moveBackward = true;
	moveLeft = true;
	moveRight = true;

	//Go through every opaque/visible render item in layer.
	for (int i = 0; i < mRitemLayer[static_cast<int>(RenderLayer::Opaque)].size(); i++)
	{
		//Check if bounding box of render item intersects with camera position + direction.
		if (mRitemLayer[static_cast<int>(RenderLayer::Opaque)][i]->bounding_box.Intersects(m_Camera.GetPosition(), m_Camera.GetLook(), temp_cam)) {
			
			//Check if our distance is less than camera collision distance.
			if (temp_cam < camera_collision_distance) {
				
				//Set movement boolean to false (this gets returned).
				moveForward = false;
			}
		}
		if (mRitemLayer[static_cast<int>(RenderLayer::Opaque)][i]->bounding_box.Intersects(m_Camera.GetPosition(), m_Camera.GetLook() * -1.0f, temp_cam)) {
			if (temp_cam < camera_collision_distance) {
				moveBackward= false;
			}
		}
		if (mRitemLayer[static_cast<int>(RenderLayer::Opaque)][i]->bounding_box.Intersects(m_Camera.GetPosition(), m_Camera.GetRight() * -1.0f, temp_cam)) {
			if (temp_cam < camera_collision_distance) {
				moveLeft = false;
			}
		}
		if (mRitemLayer[static_cast<int>(RenderLayer::Opaque)][i]->bounding_box.Intersects(m_Camera.GetPosition(),  m_Camera.GetRight(), temp_cam)) {
			if (temp_cam < camera_collision_distance) {
				moveRight = false;
			}
		}
	}
}

void CastleApp::OnKeyboardInput(const GameTimer& gt)
{
	//step3: we handle keyboard input to move the camera:

	const float dt = gt.DeltaTime();
	//Here we check all opaque render items 
	GetMovementBooleans(moveForward, moveBackward, moveLeft, moveRight);
	
	//GetAsyncKeyState returns a short (2 bytes)
	if((GetAsyncKeyState('W') & 0x8000) && moveForward) //most significant bit (MSB) is 1 when key is pressed (1000 000 000 000)
		m_Camera.Walk(m_CameraSpeed*dt);

	if((GetAsyncKeyState('S') & 0x8000) && moveBackward)
		m_Camera.Walk(-m_CameraSpeed*dt);

	if((GetAsyncKeyState('A') & 0x8000) && moveLeft)
		m_Camera.Strafe(-m_CameraSpeed*dt);

	if((GetAsyncKeyState('D') & 0x8000) && moveRight)
		m_Camera.Strafe(m_CameraSpeed*dt);

	if((GetAsyncKeyState('Q') & 0x8000))
		m_Camera.Pedestal(m_CameraSpeed*dt);

	if((GetAsyncKeyState('E') & 0x8000))
		m_Camera.Pedestal(-m_CameraSpeed*dt);

	m_Camera.UpdateViewMatrix();
}

void CastleApp::AnimateMaterials(const GameTimer& gt)
{
	// Scroll the water material texture coordinates.
	auto waterMat = mMaterials["water"].get();

	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	waterMat->NumFramesDirty = gNumFrameResources;
}

void CastleApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void CastleApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void CastleApp::UpdateMainPassCB(const GameTimer& gt)
{
	//LIGHTING AND VIEW PROJECTION-----
	
	//Here we build view projection 
	XMMATRIX view = m_Camera.GetView();
	XMMATRIX proj = m_Camera.GetProj();

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
	//Storing view projection values
    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));

	//Camera Eye Position
    mMainPassCB.EyePosW =  m_Camera.GetPosition3f();
    mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
    mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    mMainPassCB.TotalTime = gt.TotalTime();
    mMainPassCB.DeltaTime = gt.DeltaTime();
	
	//HERE WE BUILD LIGHTING!
	
	//Ambient Lighting
    mMainPassCB.AmbientLight = { 0.7f, 0.5f, 0.7f, 1.0f };
	
	//0 - 1 Directional Lighting
    mMainPassCB.Lights[0].Direction = { 0.5f, -0.6f, 0.5f };
    mMainPassCB.Lights[0].Strength = { 1.88f, 1.24f, 2.44f };
	
	mMainPassCB.Lights[1].Direction = { -0.65f, -0.65f, -0.5f };
	mMainPassCB.Lights[1].Strength = { 0.94f, 0.72f, 1.22f };
	
	
	//2 - 11 Point Light
	mMainPassCB.Lights[2].Position = { -19.0f, 17.0f, -19.0f };
	mMainPassCB.Lights[2].Strength = { 4.7f, 3.1f, 6.6f };
    mMainPassCB.Lights[3].Position = { -19.0f, 17.0f, -19.0f };
    mMainPassCB.Lights[3].Strength = { 4.7f, 3.1f, 6.6f };
	mMainPassCB.Lights[4].Position = { -19.0f, 17.0f, 19.0f };
	mMainPassCB.Lights[4].Strength = { 4.7f, 3.1f, 6.6f };
	mMainPassCB.Lights[5].Position = { 19.0f, 17.0f, 19.0f };
	mMainPassCB.Lights[5].Strength = { 4.7f, 3.1f, 6.6f };
	mMainPassCB.Lights[6].Position = { 19.0f, 17.0f, -19.0f };
	mMainPassCB.Lights[6].Strength = { 4.7f, 3.1f, 6.6f };
	mMainPassCB.Lights[7].Position = { 0.0f, 14.0f, -27.0f };
	mMainPassCB.Lights[7].Strength = { 4.7f, 3.1f, 6.6f };
	//Lightning Bolts - Additional modifers
	mMainPassCB.Lights[8].Position = { -13.0f, 6.f, 8.0f };
	mMainPassCB.Lights[8].Strength = { 4.7f, 3.1f, 6.6f };
	mMainPassCB.Lights[8].FalloffStart = 1;
	mMainPassCB.Lights[8].FalloffEnd = 5;
	mMainPassCB.Lights[8].SpotPower = 0.01;
	mMainPassCB.Lights[9].Position = { 10.0f, 6.f, 50.0f };
	mMainPassCB.Lights[9].Strength = { 4.7f, 3.1f, 6.6f };
	mMainPassCB.Lights[9].FalloffStart = 4;
	mMainPassCB.Lights[9].FalloffEnd = 12;
	mMainPassCB.Lights[9].SpotPower = 0.1;
	//Torch Lights
	mMainPassCB.Lights[10].Position = { 3.0f, 6.f, -25.0f };
	mMainPassCB.Lights[10].Strength = { 4.7f, 1.f, 1.f };
	mMainPassCB.Lights[10].FalloffStart = 1;
	mMainPassCB.Lights[10].FalloffEnd = 1.1;
	mMainPassCB.Lights[11].Position = { -3.0f, 6.f, -25.0f };
	mMainPassCB.Lights[11].Strength = { 4.7f, 1.f, 1.f };
	mMainPassCB.Lights[11].FalloffStart = 1;
	mMainPassCB.Lights[11].FalloffEnd = 1.1;
	mMainPassCB.Lights[12].Position = { 0, 10, -77};
	mMainPassCB.Lights[12].Strength = { 4.7f, 3.1f, 6.6f };
	//13- Spotlight-Pyramid
	mMainPassCB.Lights[13].Position = { 0.0f, 25.0f, 0.0f };
	mMainPassCB.Lights[13].Strength = { 4.7f, 3.1f, 6.6f };
	

	
    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

void CastleApp::UpdateWaves(const GameTimer& gt)
{
	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if ((mTimer.TotalTime() - t_base) >= 0.05f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.5f, 1.f);
		//Commenting out disturb - waves poke through bottom
		mWaves->Disturb(i, j, r);
	}

	// Update the wave simulation.
	mWaves->Update(gt.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for (int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);

		// Derive tex-coords from position by 
		// mapping [-w/2,w/2] --> [0,1]
		v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
		v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void CastleApp::LoadTextures()
{
	//Loading textures from files.
	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"../Texture/grass.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), grassTex->Filename.c_str(),
		grassTex->Resource, grassTex->UploadHeap));
	//Water Texture
	auto waterTex = std::make_unique<Texture>();
	waterTex->Name = "waterTex";
	waterTex->Filename = L"../Texture/water3.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), waterTex->Filename.c_str(),
		waterTex->Resource, waterTex->UploadHeap));
	//Wall Texture
	auto wallTex = std::make_unique<Texture>();
	wallTex->Name = "wallTex";
	wallTex->Filename = L"../Texture/walltex2.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), wallTex->Filename.c_str(),
		wallTex->Resource, wallTex->UploadHeap));
	//Dark Earthy Texture - Tower Tops
	auto earthTex = std::make_unique<Texture>();
	earthTex->Name = "earthTex";
	earthTex->Filename = L"../Texture/darkearth2.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), earthTex->Filename.c_str(),
		earthTex->Resource, earthTex->UploadHeap));
	//Gold Texture
	auto goldTex = std::make_unique<Texture>();
	goldTex->Name = "goldTex";
	goldTex->Filename = L"../Texture/gold.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), goldTex->Filename.c_str(),
		goldTex->Resource, goldTex->UploadHeap));
	//Lightning Texture
	auto rock01Tex = std::make_unique<Texture>();
	rock01Tex->Name = "rock01Tex";
	rock01Tex->Filename = L"../Texture/lightning.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), rock01Tex->Filename.c_str(),
		rock01Tex->Resource, rock01Tex->UploadHeap));
	//Placeholder texture - for future use.
	auto rock02Tex = std::make_unique<Texture>();
	rock02Tex->Name = "rock02Tex";
	rock02Tex->Filename = L"../Texture/lightning.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), rock02Tex->Filename.c_str(),
		rock02Tex->Resource, rock02Tex->UploadHeap));
	//Placeholder texture - for future use
	auto weird1Tex = std::make_unique<Texture>();
	weird1Tex->Name = "weird1Tex";
	weird1Tex->Filename = L"../Texture/lightning.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), weird1Tex->Filename.c_str(),
		weird1Tex->Resource, weird1Tex->UploadHeap));
	//Not used - green interesting material - can be changed for future projects
	auto weird2Tex = std::make_unique<Texture>();
	weird2Tex->Name = "weird2Tex";
	weird2Tex->Filename = L"../Texture/weirdtex2.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), weird2Tex->Filename.c_str(),
		weird2Tex->Resource, weird2Tex->UploadHeap));
	//Grey/Beige Pattern used for spheres
	auto weird3Tex = std::make_unique<Texture>();
	weird3Tex->Name = "weird3Tex";
	weird3Tex->Filename = L"../Texture/weirdtex3.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), weird3Tex->Filename.c_str(),
		weird3Tex->Resource, weird3Tex->UploadHeap));
	//Door texture - name needs to be changed.
	auto emeraldTex = std::make_unique<Texture>();
	emeraldTex->Name = "emeraldTex";
	emeraldTex->Filename = L"../Texture/door.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), emeraldTex->Filename.c_str(),
		emeraldTex->Resource, emeraldTex->UploadHeap));
	//Tree texture
	auto treeArrayTex = std::make_unique<Texture>();
	treeArrayTex->Name = "treeArrayTex";
	treeArrayTex->Filename = L"../Texture/treeArray.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), treeArrayTex->Filename.c_str(),
		treeArrayTex->Resource, treeArrayTex->UploadHeap));


	//Sending our textures
	mTextures[grassTex->Name] = std::move(grassTex);
	mTextures[waterTex->Name] = std::move(waterTex);
	mTextures[wallTex->Name] = std::move(wallTex);
	mTextures[earthTex->Name] = std::move(earthTex);
	mTextures[goldTex->Name] = std::move(goldTex);
	mTextures[rock01Tex->Name] = std::move(rock01Tex);
	mTextures[rock02Tex->Name] = std::move(rock02Tex);
	mTextures[weird1Tex->Name] = std::move(weird1Tex);
	mTextures[weird2Tex->Name] = std::move(weird2Tex);
	mTextures[weird3Tex->Name] = std::move(weird3Tex);
	mTextures[emeraldTex->Name] = std::move(emeraldTex);
	mTextures[treeArrayTex->Name] = std::move(treeArrayTex);

}

void CastleApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void CastleApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 12;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	
	auto grassTex = mTextures["grassTex"]->Resource;
	auto waterTex = mTextures["waterTex"]->Resource;
	auto wallTex = mTextures["wallTex"]->Resource;
	auto earthTex = mTextures["earthTex"]->Resource;
	auto goldTex = mTextures["goldTex"]->Resource;
	auto rock01Tex = mTextures["rock01Tex"]->Resource;
	auto rock02Tex = mTextures["rock02Tex"]->Resource;
	auto weird1Tex = mTextures["weird1Tex"]->Resource;
	auto weird2Tex = mTextures["weird2Tex"]->Resource;
	auto weird3Tex = mTextures["weird3Tex"]->Resource;
	auto emeraldTex = mTextures["emeraldTex"]->Resource;
	auto treeArrayTex = mTextures["treeArrayTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	srvDesc.Format = grassTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = waterTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(waterTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = wallTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(wallTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = earthTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(earthTex.Get(), &srvDesc, hDescriptor);
	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = goldTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(goldTex.Get(), &srvDesc, hDescriptor);

	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = rock01Tex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(rock01Tex.Get(), &srvDesc, hDescriptor);
	
	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = rock02Tex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(rock02Tex.Get(), &srvDesc, hDescriptor);
	
	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = weird1Tex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(weird1Tex.Get(), &srvDesc, hDescriptor);
	
	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = weird2Tex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(weird2Tex.Get(), &srvDesc, hDescriptor);
	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = weird3Tex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(weird3Tex.Get(), &srvDesc, hDescriptor);
	
	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = emeraldTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(emeraldTex.Get(), &srvDesc, hDescriptor);
	
	// next descriptor
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	auto desc = treeArrayTex->GetDesc();
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Format = treeArrayTex->GetDesc().Format;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
	md3dDevice->CreateShaderResourceView(treeArrayTex.Get(), &srvDesc, hDescriptor);



}

void CastleApp::BuildShadersAndInputLayouts()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders["treeSpriteVS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["treeSpriteGS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["treeSpritePS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mStdInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mTreeSpriteInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

//Build Water And Build Land Functions
void CastleApp::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(80.0f, 80.0f, 10, 10);

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.
	//

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		//Setting Manually
		vertices[i].Pos.y = 0.1;// GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;
	
	BoundingBox bounding_box;
	mGeometries["landGeo"] = std::move(geo);
}

void CastleApp::BuildWavesGeometry()
{
	std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
	assert(mWaves->VertexCount() < 0x0000ffff);

	// Iterate over each quad.
	int m = mWaves->RowCount();
	int n = mWaves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	UINT vbByteSize = mWaves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	// Set dynamically.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["waterGeo"] = std::move(geo);
}

void CastleApp::BuildGeometry()
{
	//Create our meshdata
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 3);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.f, 20, 20);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(50.0f, 50.0f, 25, 25);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.5f, 1, 10, 10);
	GeometryGenerator::MeshData cone = geoGen.CreateCone(1, 1, 30, 20);
	GeometryGenerator::MeshData wedge = geoGen.CreateWedge(1.f, 1.f, 1.f, 3);
	GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(1.f, 1.f, 10.f, 10);
	GeometryGenerator::MeshData truncatedcone = geoGen.CreateTruncatedCone(1.f, 1.f, 100);
	GeometryGenerator::MeshData truncatedpyramid = geoGen.CreateTruncatedPyramid(1.f, 1.f, 100, 20);

	// Vertex Cache
	UINT boxVertexOffset = 0;
	UINT sphereVertexOffset = boxVertexOffset + (UINT)box.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
	UINT coneVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();
	UINT pyramidVertexOffset = coneVertexOffset + (UINT)cone.Vertices.size();
	UINT wedgeVertexOffset = pyramidVertexOffset + (UINT)pyramid.Vertices.size();
	UINT truncatedConeVertexOffset = wedgeVertexOffset + (UINT)wedge.Vertices.size();
	UINT truncatedPyramidVertexOffset = truncatedConeVertexOffset + (UINT)truncatedcone.Vertices.size();

	//Index Cache
	UINT boxIndexOffset = 0;
	UINT sphereIndexOffset = boxIndexOffset + (UINT)box.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
	UINT coneIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();
	UINT pyramidIndexOffset = coneIndexOffset + (UINT)cone.Indices32.size();
	UINT wedgeIndexOffset = pyramidIndexOffset + (UINT)pyramid.Indices32.size();
	UINT truncatedConeIndexOffset = wedgeIndexOffset + (UINT)wedge.Indices32.size();
	UINT truncatedPyramidIndexOffset = truncatedConeIndexOffset + (UINT)truncatedcone.Indices32.size();
	//Create Submesh Geometry 
	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	SubmeshGeometry coneSubmesh;
	coneSubmesh.IndexCount = (UINT)cone.Indices32.size();
	coneSubmesh.StartIndexLocation = coneIndexOffset;
	coneSubmesh.BaseVertexLocation = coneVertexOffset;


	SubmeshGeometry pyramidSubmesh;
	pyramidSubmesh.IndexCount = (UINT)pyramid.Indices32.size();
	pyramidSubmesh.StartIndexLocation = pyramidIndexOffset;
	pyramidSubmesh.BaseVertexLocation = pyramidVertexOffset;

	SubmeshGeometry wedgeSubmesh;
	wedgeSubmesh.IndexCount = (UINT)wedge.Indices32.size();
	wedgeSubmesh.StartIndexLocation = wedgeIndexOffset;
	wedgeSubmesh.BaseVertexLocation = wedgeVertexOffset;


	SubmeshGeometry truncatedConeSubmesh;
	truncatedConeSubmesh.IndexCount = (UINT)truncatedcone.Indices32.size();
	truncatedConeSubmesh.StartIndexLocation = truncatedConeIndexOffset;
	truncatedConeSubmesh.BaseVertexLocation = truncatedConeVertexOffset;

	SubmeshGeometry truncatedPyramidSubmesh;
	truncatedPyramidSubmesh.IndexCount = (UINT)truncatedpyramid.Indices32.size();
	truncatedPyramidSubmesh.StartIndexLocation = truncatedPyramidIndexOffset;
	truncatedPyramidSubmesh.BaseVertexLocation = truncatedPyramidVertexOffset;
	//Add together vertexes.
	auto totalVertexCount =
		box.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size() +
		cone.Vertices.size() +
		pyramid.Vertices.size() +
		wedge.Vertices.size() +
			truncatedcone.Vertices.size() +
				truncatedpyramid.Vertices.size();
		
	//Create vector of vertex count
	std::vector<Vertex> vertices(totalVertexCount);
	
	//Declare k increment outside of scope to keep track of vertices size.
	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		auto& p = box.Vertices[i].Position;
		vertices[k].Pos = p;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::Crimson);
	}
	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::Crimson);
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::SteelBlue);
	}
	for (size_t i = 0; i < cone.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cone.Vertices[i].Position;
		vertices[k].Normal = cone.Vertices[i].Normal;
		vertices[k].TexC = cone.Vertices[i].TexC;
		// vertices[k].Color = XMFLOAT4(DirectX::Colors::Blue);
	}
	for (size_t i = 0; i < pyramid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = pyramid.Vertices[i].Position;
		vertices[k].Normal = pyramid.Vertices[i].Normal;
		vertices[k].TexC = pyramid.Vertices[i].TexC;
		// vertices[k].Color = XMFLOAT4(DirectX::Colors::Blue);
	}
	for (size_t i = 0; i < wedge.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = wedge.Vertices[i].Position;
		vertices[k].Normal = wedge.Vertices[i].Normal;
		vertices[k].TexC = wedge.Vertices[i].TexC;
		// vertices[k].Color = XMFLOAT4(DirectX::Colors::Blue);
	}
	for (size_t i = 0; i < truncatedcone.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = truncatedcone.Vertices[i].Position;
		vertices[k].Normal = truncatedcone.Vertices[i].Normal;
		vertices[k].TexC = truncatedcone.Vertices[i].TexC;
		// vertices[k].Color = XMFLOAT4(DirectX::Colors::Blue);
	}
	for (size_t i = 0; i < truncatedpyramid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = truncatedpyramid.Vertices[i].Position;
		vertices[k].Normal = truncatedpyramid.Vertices[i].Normal;
		vertices[k].TexC = truncatedpyramid.Vertices[i].TexC;
		// vertices[k].Color = XMFLOAT4(DirectX::Colors::Blue);
	}
	//Insert indices into indices index vector
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(cone.GetIndices16()), std::end(cone.GetIndices16()));
	indices.insert(indices.end(), std::begin(pyramid.GetIndices16()), std::end(pyramid.GetIndices16()));
	indices.insert(indices.end(), std::begin(wedge.GetIndices16()), std::end(wedge.GetIndices16()));
	indices.insert(indices.end(), std::begin(truncatedcone.GetIndices16()), std::end(truncatedcone.GetIndices16()));
	indices.insert(indices.end(), std::begin(truncatedpyramid.GetIndices16()), std::end(truncatedpyramid.GetIndices16()));
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	
	//Create meshgeo var
	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;
	
	//Draw submesh arguments
	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;
	geo->DrawArgs["cone"] = coneSubmesh;
	geo->DrawArgs["pyramid"] = pyramidSubmesh;
	geo->DrawArgs["wedge"] = wedgeSubmesh;
	geo->DrawArgs["truncatedcone"] = truncatedConeSubmesh;
	geo->DrawArgs["truncatedpyramid"] = truncatedPyramidSubmesh;
	//Send
	mGeometries[geo->Name] = std::move(geo);
}

void CastleApp::BuildTreeSpritesGeometry()
{
	//step5
	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int treeCount = 16;
	std::array<TreeSpriteVertex, 16> vertices;
	for (UINT i = 1; i < treeCount; ++i)
	{
		float x = 0;
		float z = 0;
		float y = 0;
		switch (i) {
		case 1:
			x = -18;
			y = 13;
			z = -33;
			break;
		case 2:
			x = -5;
			y = 13;
			z = -28;
			break;
		case 3:
			x = 10;
			y = 13;
			z = -35;
			break;
		case 4:
			x = -38;
			y = 11;
			z = 25;
			break;
		case 5:
			x = -35;
			y = 11;
			z = -30;
			break;
		case 6:
			x = -35;
			y = 14;
			z = 10;
			break;
		case 7:
			x = 32;
			y = 12;
			z = 0;
			break;

		case 8:
			x = 35;
			y = 12;
			z = -38;
			break;

		case 9:
			x = 35;
			y = 13;
			z = 30;
			break;
		case 10:
			x = 7;
			y = 11;
			z = 47;
			break;
		case 11:
			//Left of 21
			//4.5, 10, -63
			x = 2.5;
			y = 10;
			z = -63;
			break;
		case 12:
			//Above 24
			//-5, 10, -53.5
			x = -5;
			y = 10;
			z = -50.5;
			break;
		case 13:
			//In front Cylinder left
			//7, 12.5, -76.5
			x = 7;
			y = 11;
			z = -79.5;
			break;
		
		case 14:
			//In front Cylinder right
			//-7, 12.5, -76.5
			x = -7;
			y = 11;
			z = -79.5;
			break;
		case 15:
			//Above 35
			//-21.5, 10, -66.5
			x = -21.5;
			y = 10;
			z = -64.5;
			break;
		case 16:
			//Above 27
			//-12, 10, -49
			x = -12;
			y = 10;
			z = -47;
			break;
		default:
			
			break;
		}
		
		

		// Move tree slightly above land height.
		//y = 9.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
	}

	std::array<std::uint16_t, 16> indices =
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "treeSpritesGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	mGeometries["treeSpritesGeo"] = std::move(geo);
}
//Creating separate build for lightning sprites.
void CastleApp::BuildLightningSpritesGeometry()
{
	//step5
	struct LightningSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int treeCount = 2;
	std::array<LightningSpriteVertex, 2> vertices;
	for (UINT i = 0; i < treeCount; ++i)
	{

		// Move tree slightly above land height.
		
		vertices[0].Pos = XMFLOAT3(10, 50, 50);
		vertices[0].Size = XMFLOAT2(25.0f, 100.0f);

		vertices[1].Pos = XMFLOAT3(-13, 30, 8);
		vertices[1].Size = XMFLOAT2(10.0f, 55.0f);
	}

	std::array<std::uint16_t, 16> indices =
	{
		0,
		1
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(LightningSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "lightningSpritesGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(LightningSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	mGeometries["lightningSpritesGeo"] = std::move(geo);
}
void CastleApp::BuildPSOs()
{

	//4 PSOS - Opaque, Transparent, AlphaTested, AlphaTested-Treesprites
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	
	// PSO opaque objects.
	
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mStdInputLayout.data(), (UINT)mStdInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;

	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
	
	// PSO transparent objects
	
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//transparentPsoDesc.BlendState.AlphaToCoverageEnable = true;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

	
	// PSO alpha tested objects

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));
	
	// PSO tree sprites
	
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	treeSpritePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteVS"]->GetBufferPointer()),
		mShaders["treeSpriteVS"]->GetBufferSize()
	};
	treeSpritePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteGS"]->GetBufferPointer()),
		mShaders["treeSpriteGS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpritePS"]->GetBufferPointer()),
		mShaders["treeSpritePS"]->GetBufferSize()
	};

	
	//step1
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs["treeSprites"])));
}

void CastleApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
	}
}

void CastleApp::BuildMaterials()
{
	//Build Material Definitions
	
	int i = 0;
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = i;
	grass->DiffuseSrvHeapIndex = i;
	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;
	i++;
	// This is not a good water material definition, but we do not have all the rendering
	// tools we need (transparency, environment reflection), so we fake it for now.
	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = i;
	water->DiffuseSrvHeapIndex = i;
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.0f;
	//Incrementing material location
	i++;
	//Default
	auto wirefence = std::make_unique<Material>();
	wirefence->Name = "wall";
	wirefence->MatCBIndex = i;
	wirefence->DiffuseSrvHeapIndex = i;
	wirefence->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wirefence->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	wirefence->Roughness = 0.25f;
	//Incrementing material location
	i++;
	auto stone = std::make_unique<Material>();
	stone->Name = "stone";
	stone->MatCBIndex = i;
	stone->DiffuseSrvHeapIndex = i;
	stone->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	stone->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	stone->Roughness = 0.2f;
	//Incrementing material location
	i++;
	auto gold = std::make_unique<Material>();
	gold->Name = "gold";
	gold->MatCBIndex = i;
	gold->DiffuseSrvHeapIndex = i;
	gold->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	gold->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	gold->Roughness = 0.9f;
	//Incrementing material location
	i++;
	auto earth = std::make_unique<Material>();
	earth->Name = "earth";
	earth->MatCBIndex = i;
	earth->DiffuseSrvHeapIndex = i;
	earth->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	earth->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	earth->Roughness = 0.9f;
	//Incrementing material location
	i++;
	auto rock1 = std::make_unique<Material>();
	rock1->Name = "rock1";
	rock1->MatCBIndex = i;
	rock1->DiffuseSrvHeapIndex = i;
	rock1->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	rock1->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	rock1->Roughness = 0.9f;
	//Incrementing material location
	i++;
	auto rock2 = std::make_unique<Material>();
	rock2->Name = "rock2";
	rock2->MatCBIndex = i;
	rock2->DiffuseSrvHeapIndex = i;
	rock2->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	rock2->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	rock2->Roughness = 0.9f;
	//Incrementing material location
	i++;
	auto weird1 = std::make_unique<Material>();
	weird1->Name = "weird1";
	weird1->MatCBIndex = i;
	weird1->DiffuseSrvHeapIndex = i;
	weird1->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	weird1->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	weird1->Roughness = 0.9f;
	//Incrementing material location
	i++;
	auto weird2 = std::make_unique<Material>();
	weird2->Name = "weird2";
	weird2->MatCBIndex = i;
	weird2->DiffuseSrvHeapIndex = i;
	weird2->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	weird2->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	weird2->Roughness = 0.9f;
	//Incrementing material location
	i++;
	auto weird3 = std::make_unique<Material>();
	weird3->Name = "weird3";
	weird3->MatCBIndex = i;
	weird3->DiffuseSrvHeapIndex = i;
	weird3->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	weird3->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	weird3->Roughness = 0.9f;
	//Incrementing material location
	i++;
	auto treeSprites = std::make_unique<Material>();
	treeSprites->Name = "treeSprites";
	treeSprites->MatCBIndex = i;
	treeSprites->DiffuseSrvHeapIndex = i;
	treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	treeSprites->Roughness = 0.125f;


	//Send them over
	mMaterials["grass"] = std::move(grass);
	mMaterials["water"] = std::move(water);
	mMaterials["wall"] = std::move(wirefence);
	mMaterials["stone"] = std::move(stone);
	mMaterials["gold"] = std::move(gold);
	mMaterials["earth"] = std::move(earth);
	mMaterials["rock1"] = std::move(rock1);
	mMaterials["rock2"] = std::move(rock2);
	mMaterials["weird1"] = std::move(weird1);
	mMaterials["weird2"] = std::move(weird2);
	mMaterials["weird3"] = std::move(weird3);
	mMaterials["treeSprites"] = std::move(treeSprites);


}

void CastleApp::CheckCollision(std::vector<XMMATRIX> positions, XMMATRIX cameraPosition)
{
	for(int i = 0; i < positions.size() - 1; i++)
	{
	}
}
//Build Shape Item That Rotates
void CastleApp::Build_Render_Item_Rotate(const char* item, XMMATRIX scale_matrix, XMMATRIX translate_matrix, XMMATRIX rotation_matrix, const char* material, UINT ObjIndex)
{
	BoundingBox bounding_box;
	auto shape_render_item = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&shape_render_item->World, scale_matrix * rotation_matrix * translate_matrix);
	shape_render_item->ObjCBIndex = ObjIndex;
	shape_render_item->Mat = mMaterials[material].get();
	shape_render_item->Geo = mGeometries["boxGeo"].get();
	shape_render_item->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	shape_render_item->IndexCount = shape_render_item->Geo->DrawArgs[item].IndexCount;
	shape_render_item->StartIndexLocation = shape_render_item->Geo->DrawArgs[item].StartIndexLocation;
	shape_render_item->BaseVertexLocation = shape_render_item->Geo->DrawArgs[item].BaseVertexLocation;
	
	mRitemLayer[(int)RenderLayer::Opaque].push_back(shape_render_item.get());
	mAllRitems.push_back(std::move(shape_render_item));
}
//Build Shape Item No Rotation
void CastleApp::Build_Render_Item(const char* item, XMMATRIX scale_matrix, XMMATRIX translate_matrix, const char* material, UINT ObjIndex)
{
	BoundingBox bounding_box;
	std::string str = item;
	auto shape_render_item = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&shape_render_item->World, scale_matrix * translate_matrix);
	shape_render_item->ObjCBIndex = ObjIndex;
	shape_render_item->Mat = mMaterials[material].get();
	shape_render_item->Geo = mGeometries["boxGeo"].get();
	shape_render_item->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	shape_render_item->IndexCount = shape_render_item->Geo->DrawArgs[item].IndexCount;
	shape_render_item->StartIndexLocation = shape_render_item->Geo->DrawArgs[item].StartIndexLocation;
	shape_render_item->BaseVertexLocation = shape_render_item->Geo->DrawArgs[item].BaseVertexLocation;
	
	mRitemLayer[(int)RenderLayer::Opaque].push_back(shape_render_item.get());
	mAllRitems.push_back(std::move(shape_render_item));
}


void CastleApp::Build_Render_Item_Collision(const char* item, XMMATRIX scale_matrix, XMMATRIX translate_matrix, const char* material, UINT ObjIndex)
{
	BoundingBox bounding_box;
	auto shape_render_item = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&shape_render_item->World, scale_matrix * translate_matrix);
	shape_render_item->ObjCBIndex = ObjIndex;
	shape_render_item->Mat = mMaterials[material].get();
	shape_render_item->Geo = mGeometries["boxGeo"].get();
	shape_render_item->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	shape_render_item->IndexCount = shape_render_item->Geo->DrawArgs[item].IndexCount;
	shape_render_item->StartIndexLocation = shape_render_item->Geo->DrawArgs[item].StartIndexLocation;
	shape_render_item->BaseVertexLocation = shape_render_item->Geo->DrawArgs[item].BaseVertexLocation;

	//Setting render items bounding box center and extents for use with directXCollision.
	XMStoreFloat3(&bounding_box.Center, XMVectorSet(XMVectorGetX(translate_matrix.r[3]), XMVectorGetY(translate_matrix.r[3]), XMVectorGetZ(translate_matrix.r[3]), 1.0f));
	XMStoreFloat3(&bounding_box.Extents, 0.5f * XMVectorSet(XMVectorGetX(scale_matrix.r[0]), XMVectorGetY(scale_matrix.r[1]), XMVectorGetZ(scale_matrix.r[2]), 1.0f));
	
	shape_render_item->bounding_box = bounding_box;



	
	mRitemLayer[(int)RenderLayer::Opaque].push_back(shape_render_item.get());
	mAllRitems.push_back(std::move(shape_render_item));
}

void CastleApp::BuildMaze(UINT& objCBIndex)
{
	//Wall One
	Build_Render_Item_Collision("box", XMMatrixScaling(2, 10.f, 8.f), XMMatrixTranslation(-6, 10, -27), "wall", objCBIndex++);
	//Wall Two
	Build_Render_Item_Collision("box", XMMatrixScaling(2, 10.f, 8.f), XMMatrixTranslation(6, 10, -27), "wall", objCBIndex++);
	//Wall Two/2
	Build_Render_Item_Collision("box", XMMatrixScaling(3, 10.f, 1.f), XMMatrixTranslation(6, 10, -28), "wall", objCBIndex++);
	//Wall Two/2
	Build_Render_Item_Collision("box", XMMatrixScaling(3, 10.f, 1.f), XMMatrixTranslation(-6, 10, -28), "wall", objCBIndex++);
	
	//Maze Cylinder Front MiddleR
	Build_Render_Item("cylinder", XMMatrixScaling(3.f, 15.f, 3.f), XMMatrixTranslation(-7, 12.5, -76.5), "weird2", objCBIndex++);
	//Maze Cylinder Front MiddleL
	Build_Render_Item("cylinder", XMMatrixScaling(3.f, 15.f, 3.f), XMMatrixTranslation(7, 12.5, -76.5), "weird2", objCBIndex++);

	//Inner Sphere MiddleR
	Build_Render_Item("sphere", XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(-7, 20, -76.5), "stone", objCBIndex++);
	//Inner Sphere MiddleL
	Build_Render_Item("sphere", XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(7, 20, -76.5), "stone", objCBIndex++);

	
	//Maze Cylinder Front MiddleR
	Build_Render_Item("cylinder", XMMatrixScaling(3.f, 15.f, 3.f), XMMatrixTranslation(-30, 12.5, -76.5), "weird2", objCBIndex++);
	//Maze Cylinder Front MiddleL
	Build_Render_Item("cylinder", XMMatrixScaling(3.f, 15.f, 3.f), XMMatrixTranslation(30, 12.5, -76.5), "weird2", objCBIndex++);

	//Inner Sphere MiddleR
	Build_Render_Item("sphere", XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(-30, 20, -76.5), "stone", objCBIndex++);
	//Inner Sphere MiddleL
	Build_Render_Item("sphere", XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(30, 20, -76.5), "stone", objCBIndex++);
	
	// Maze Ground
	Build_Render_Item_Collision("box", XMMatrixScaling(150, 5.f, 200.f), XMMatrixTranslation(0, 2.5, 0), "grass", objCBIndex++);
	// Front Wedge
	Build_Render_Item_Collision("wedge", XMMatrixScaling(150, 5.f, 10.f), XMMatrixTranslation(0, 2.5, -105), "grass", objCBIndex++);
	// Left Wedge
	Build_Render_Item_Rotate("wedge", XMMatrixScaling(200, 5.f, 10.f), XMMatrixTranslation(-80, 2.5, 0), XMMatrixRotationY(1.5707963268), "grass", objCBIndex++);
	// Right Wedge
	Build_Render_Item_Rotate("wedge", XMMatrixScaling(200, 5.f, 10.f), XMMatrixTranslation(80, 2.5, 0), XMMatrixRotationY(4.7123889804), "grass", objCBIndex++);
	//Maze 1 ( Back Left Wall )
	Build_Render_Item_Collision("box", XMMatrixScaling(24.f, 10.f, 1.f), XMMatrixTranslation(-18.5, 10, -30.5), "wall", objCBIndex++);
	//Maze 2 ( Back Right wall )
	Build_Render_Item_Collision("box", XMMatrixScaling(24.f, 10.f, 1.f), XMMatrixTranslation(18.5, 10, -30.5), "wall", objCBIndex++);
	//Maze 3 ( Outer Left Wall )
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 46.f), XMMatrixTranslation(-30, 10, -54), "wall", objCBIndex++);
	//Maze 4 ( Outer Right Wall )
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 46.f), XMMatrixTranslation(30, 10, -54), "wall", objCBIndex++);
	// Maze 5 ( front Left Wall )
	Build_Render_Item_Collision("box", XMMatrixScaling(24.f, 10.f, 1.f), XMMatrixTranslation(-18.5, 10, -76.5), "wall", objCBIndex++);
	//Maze 2 ( Front Right wall )
	Build_Render_Item_Collision("box", XMMatrixScaling(24.f, 10.f, 1.f), XMMatrixTranslation(18.5, 10, -76.5), "wall", objCBIndex++);
	//#5 - Difference of 4.5
	Build_Render_Item_Collision("box", XMMatrixScaling(51.f, 10.f, 1.f), XMMatrixTranslation(0, 10, -71.5), "wall", objCBIndex++);
	//#6 - Difference of 4.5
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 14.f), XMMatrixTranslation(25, 10, -65), "wall", objCBIndex++);
	//#7 - Difference of 4.5
	Build_Render_Item_Collision("box", XMMatrixScaling(6.f, 10.f, 1.f), XMMatrixTranslation(22, 10, -58.5), "wall", objCBIndex++);
	//#8
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 8.f), XMMatrixTranslation(19.5, 10, -63), "wall", objCBIndex++);
	//#9 - Difference of 4.5
	Build_Render_Item_Collision("box", XMMatrixScaling(6.f, 10.f, 1.f), XMMatrixTranslation(17, 10, -66.5), "wall", objCBIndex++);
	//#10
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 8.f), XMMatrixTranslation(14.5, 10, -62), "wall", objCBIndex++);
	//#11
	Build_Render_Item_Collision("box", XMMatrixScaling(16.f, 10.f, 1.f), XMMatrixTranslation(22, 10, -53.5), "wall", objCBIndex++);
	//#12
	Build_Render_Item_Collision("box", XMMatrixScaling(12.f, 10.f, 1.f), XMMatrixTranslation(20, 10, -48.5), "wall", objCBIndex++);
	//#13
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 8.f), XMMatrixTranslation(26, 10, -45), "wall", objCBIndex++);
	//--Addition1
	Build_Render_Item_Collision("box", XMMatrixScaling(6.f, 10.f, 1.f), XMMatrixTranslation(24, 10, -36), "wall", objCBIndex++);
	//--Addition2
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 6.f), XMMatrixTranslation(20.5, 10, -38.5), "wall", objCBIndex++);
	//--Addition3
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 6.f), XMMatrixTranslation(20.5, 10, -33.5), "wall", objCBIndex++);
	//#14
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 14.f), XMMatrixTranslation(14.5, 10, -42), "wall", objCBIndex++);
	//#15-
	Build_Render_Item_Collision("box", XMMatrixScaling(6.f, 10.f, 1.f), XMMatrixTranslation(12, 10, -36), "wall", objCBIndex++);
	//#16-
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 13.5f), XMMatrixTranslation(9.5, 10, -42), "wall", objCBIndex++);
	//#17
	Build_Render_Item_Collision("box", XMMatrixScaling(8.f, 10.f, 1.f), XMMatrixTranslation(6, 10, -48.5), "wall", objCBIndex++);
	//#18
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 16.f), XMMatrixTranslation(2, 10, -46), "wall", objCBIndex++);
	//#19
	Build_Render_Item_Collision("box", XMMatrixScaling(8.f, 10.f, 1.f), XMMatrixTranslation(6, 10, -53.5), "wall", objCBIndex++);
	//#20
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 14.f), XMMatrixTranslation(9.5, 10, -60.), "wall", objCBIndex++);
	//#21
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 8.f), XMMatrixTranslation(4.5, 10, -63), "wall", objCBIndex++);
	//#22
	Build_Render_Item_Collision("box", XMMatrixScaling(6.f, 10.f, 1.f), XMMatrixTranslation(1, 10, -59.5), "wall", objCBIndex++);
	//#23
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 16.f), XMMatrixTranslation(-2.5, 10, -52.), "wall", objCBIndex++);
	//#EXTRA
	Build_Render_Item_Collision("box", XMMatrixScaling(10.f, 10.f, 1.f), XMMatrixTranslation(-3, 10, -38.5), "wall", objCBIndex++);
	//#24
	Build_Render_Item_Collision("box", XMMatrixScaling(5.f, 10.f, 1.f), XMMatrixTranslation(-5, 10, -53.5), "wall", objCBIndex++);
	//#25
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 10.f), XMMatrixTranslation(-7, 10, -49), "wall", objCBIndex++);
	//#26
	Build_Render_Item_Collision("box", XMMatrixScaling(5.f, 10.f, 1.f), XMMatrixTranslation(-9.5, 10, -44.5), "wall", objCBIndex++);
	//#27
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 10.f), XMMatrixTranslation(-12, 10, -49), "wall", objCBIndex++);
	//#28
	Build_Render_Item_Collision("box", XMMatrixScaling(12.f, 10.f, 1.f), XMMatrixTranslation(-19, 10, -38.5), "wall", objCBIndex++);
	//#29 
	Build_Render_Item_Collision("box", XMMatrixScaling(5.f, 10.f, 1.f), XMMatrixTranslation(-15, 10, -53.5), "wall", objCBIndex++);
	//#30
	Build_Render_Item_Collision("box", XMMatrixScaling(11.f, 10.f, 1.f), XMMatrixTranslation(-1, 10, -66.5), "wall", objCBIndex++);
	//#31
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 8.f), XMMatrixTranslation(-7, 10, -63), "wall", objCBIndex++);
	//#32
	Build_Render_Item_Collision("box", XMMatrixScaling(5.f, 10.f, 1.f), XMMatrixTranslation(-9.5, 10, -59.5), "wall", objCBIndex++);
	//#33
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 8.f), XMMatrixTranslation(-12, 10, -63), "wall", objCBIndex++);
	//#34
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 22.f), XMMatrixTranslation(-18, 10, -56), "wall", objCBIndex++);
	//#35
	Build_Render_Item_Collision("box", XMMatrixScaling(8.f, 10.f, 1.f), XMMatrixTranslation(-21.5, 10, -66.5), "wall", objCBIndex++);
	//#36
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 10.f), XMMatrixTranslation(-25, 10, -62), "wall", objCBIndex++);
	//#37
	Build_Render_Item_Collision("box", XMMatrixScaling(8.f, 10.f, 1.f), XMMatrixTranslation(-25.5, 10, -53.5), "wall", objCBIndex++);
	//#38
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 8.f), XMMatrixTranslation(-25, 10, -42), "wall", objCBIndex++);
	//EXTRA2
	Build_Render_Item_Collision("box", XMMatrixScaling(4.f, 10.f, 1.f), XMMatrixTranslation(-23.5, 10, -45.5), "wall", objCBIndex++);
	//EXTRA3
	Build_Render_Item_Collision("box", XMMatrixScaling(4.f, 10.f, 1.f), XMMatrixTranslation(-23.5, 10, -57.5), "wall", objCBIndex++);
	//EXTRA 4
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 4.f), XMMatrixTranslation(-25, 10, -48), "wall", objCBIndex++);
	//#FINAL PIECE
	Build_Render_Item_Collision("box", XMMatrixScaling(1.f, 10.f, 8.f), XMMatrixTranslation(-7.5, 10, -34), "wall", objCBIndex++);
}


void CastleApp::BuildCastle(UINT& objCBIndex)
{
	//Front wall Left
	Build_Render_Item_Collision("box", XMMatrixScaling(20, 10.f, 2.f), XMMatrixTranslation(-12, 10, -23), "wall", objCBIndex++);
	//Front wall Right
	Build_Render_Item_Collision("box", XMMatrixScaling(20, 10.f, 2.f), XMMatrixTranslation(12, 10, -23), "wall", objCBIndex++);
	
	////Front wall Top
	Build_Render_Item_Collision("box", XMMatrixScaling(4, 2.f, 2.f), XMMatrixTranslation(0, 14, -23), "wall", objCBIndex++);
	////Back wall
	Build_Render_Item_Collision("box", XMMatrixScaling(44.f, 10.f, 2.f), XMMatrixTranslation(0, 10, 23), "wall", objCBIndex++);
	////Left wall
	Build_Render_Item_Collision("box", XMMatrixScaling(2.f, 10.f, 48.f), XMMatrixTranslation(-23, 10, 0), "wall", objCBIndex++);
	////Right wall
	Build_Render_Item_Collision("box", XMMatrixScaling(2.f, 10.f, 48.f), XMMatrixTranslation(23, 10, 0), "wall", objCBIndex++);

	//Inner front wall
	Build_Render_Item_Collision("box", XMMatrixScaling(12.f, 10.f, 2.f), XMMatrixTranslation(14, 10, -19), "wall", objCBIndex++);/**/
	Build_Render_Item_Collision("box", XMMatrixScaling(12.f, 10.f, 2.f), XMMatrixTranslation(-14, 10, -19), "wall", objCBIndex++);
	//Inner back wall
	Build_Render_Item_Collision("box", XMMatrixScaling(38.5f, 10.f, 2.f), XMMatrixTranslation(0, 10, 19), "wall", objCBIndex++);
	//Inner left wall
	Build_Render_Item_Collision("box", XMMatrixScaling(2.f, 10.f, 38.5f), XMMatrixTranslation(-19, 10, 0), "wall", objCBIndex++);
	//Inner right wall
	Build_Render_Item_Collision("box", XMMatrixScaling(2.f, 10.f, 38.5f), XMMatrixTranslation(19, 10, 0), "wall", objCBIndex++);

	////Pyramid
	Build_Render_Item_Rotate("pyramid", XMMatrixScaling(20.f, 20.f, 20.f), XMMatrixRotationY(0.7853981634), XMMatrixTranslation(0, 14.5, 0), "gold", objCBIndex++);

	////Wedges
	for (int i = 0; i < 44; ++i)
	{
		if (i % 2 == 0)
		{
			//Wedge front
			Build_Render_Item("wedge", XMMatrixScaling(1.f, 1.f, 1.f), XMMatrixTranslation(-21 + i, 15.5, -23), "weird1", objCBIndex++);
			//Wedge back
			Build_Render_Item_Rotate("wedge", XMMatrixScaling(1.f, 1.f, 1.f), XMMatrixTranslation(-21 + i, 15.5, 23),  XMMatrixRotationY(3.1415926536), "weird1", objCBIndex++);
			////Wedge left
			Build_Render_Item_Rotate("wedge", XMMatrixScaling(1.f, 1.f, 1.f), XMMatrixTranslation(-23, 15.5, -21 + i), XMMatrixRotationY(1.5707963268),  "weird1", objCBIndex++);
			//Wedge right
			Build_Render_Item_Rotate("wedge", XMMatrixScaling(1.f, 1.f, 1.f), XMMatrixTranslation(23, 15.5, -21 + i), XMMatrixRotationY(4.7123889804),  "weird1", objCBIndex++);
		}

	}

	//Outer Cylinder Front Left
	Build_Render_Item("cylinder", XMMatrixScaling(4.f, 15.f, 4.f), XMMatrixTranslation(-23, 12.5, -23), "weird2", objCBIndex++);
	//Outer Cylinder Front Right
	Build_Render_Item("cylinder", XMMatrixScaling(4.f, 15.f, 4.f), XMMatrixTranslation(23, 12.5, -23), "weird2", objCBIndex++);
	//Outer Cylinder Back Left
	Build_Render_Item("cylinder", XMMatrixScaling(4.f, 15.f, 4.f), XMMatrixTranslation(-23, 12.5, 23), "weird2", objCBIndex++);
	//Outer Cylinder Back Right
	Build_Render_Item("cylinder", XMMatrixScaling(4.f, 15.f, 4.f), XMMatrixTranslation(23, 12.5, 23), "weird2", objCBIndex++);


	//Inner Cylinder Front Left
	Build_Render_Item("cylinder", XMMatrixScaling(2.f, 15.f, 2.f), XMMatrixTranslation(-19, 12.5, -19), "weird2", objCBIndex++);
	//Inner Cylinder Front Right
	Build_Render_Item("cylinder", XMMatrixScaling(2.f, 15.f, 2.f), XMMatrixTranslation(19, 12.5, -19), "weird2", objCBIndex++);
	//Inner Cylinder Front MiddleR
	Build_Render_Item("cylinder", XMMatrixScaling(3.f, 15.f, 3.f), XMMatrixTranslation(-7, 12.5, -19), "weird2", objCBIndex++);
	//Inner Cylinder Front MiddleL
	Build_Render_Item("cylinder", XMMatrixScaling(3.f, 15.f, 3.f), XMMatrixTranslation(7, 12.5, -19), "weird2", objCBIndex++);
	//Inner Cylinder Back Left
	Build_Render_Item("cylinder", XMMatrixScaling(2.f, 15.f, 2.f), XMMatrixTranslation(-19, 12.5, 19), "weird2", objCBIndex++);
	//Inner Cylinder Back Right
	Build_Render_Item("cylinder", XMMatrixScaling(2.f, 15.f, 2.f), XMMatrixTranslation(19, 12.5, 19), "weird2", objCBIndex++);



	//Outer cone Front Left
	Build_Render_Item("cone", XMMatrixScaling(4.f, 6.f, 4.f), XMMatrixTranslation(-23, 22, 23), "stone", objCBIndex++);
	//Outer cone Front Right
	Build_Render_Item("cone", XMMatrixScaling(4.f, 6.f, 4.f), XMMatrixTranslation(23, 22, -23), "stone", objCBIndex++);
	//Outer cone Back Left
	Build_Render_Item("cone", XMMatrixScaling(4.f, 6.f, 4.f), XMMatrixTranslation(-23, 22, -23), "stone", objCBIndex++);
	//Outer cone Back Right
	Build_Render_Item("cone", XMMatrixScaling(4.f, 6.f, 4.f), XMMatrixTranslation(23, 22, 23), "stone", objCBIndex++);

	//Inner Sphere Front Left
	Build_Render_Item("sphere", XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(-19, 20, -19), "weird1", objCBIndex++);
	//Inner Sphere Front Right
	Build_Render_Item("sphere", XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(19, 20, -19), "weird1", objCBIndex++);
	//Inner Sphere MiddleR
	Build_Render_Item("sphere", XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(-7, 20, -19), "gold", objCBIndex++);
	//Inner Sphere MiddleL
	Build_Render_Item("sphere", XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(7, 20, -19), "gold", objCBIndex++);
	//Inner Sphere Back Left
	Build_Render_Item("sphere", XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(-19, 20, 19), "weird1", objCBIndex++);
	//Inner Sphere Back Right
	Build_Render_Item("sphere", XMMatrixScaling(2.f, 2.f, 2.f), XMMatrixTranslation(19, 20, 19), "weird1", objCBIndex++);

	////Front Gate
	Build_Render_Item_Rotate("box", XMMatrixScaling(4.f, 8.f, 2.f), XMMatrixTranslation(0, 9, -23), XMMatrixRotationY(3.14159),  "weird3", objCBIndex++);
	//Ramp
	Build_Render_Item("wedge", XMMatrixScaling(6.f, 5.f, 8.f), XMMatrixTranslation(0, 2.5, -30),  "weird1", objCBIndex++);
	//Torch(s)
	//Torch(s)
	Build_Render_Item("cylinder", XMMatrixScaling(1.f, 1.f, 1.f), XMMatrixTranslation(3, 5.5, -25),  "weird3", objCBIndex++);
	Build_Render_Item("cylinder", XMMatrixScaling(1.f, 1.f, 1.f), XMMatrixTranslation(-3, 5.5, -25),  "weird3", objCBIndex++);
}

void CastleApp::Build_Render_Items()
{
	//Build the water
	UINT objCBIndex = 0;
	auto wavesRitem = std::make_unique<RenderItem>();
	wavesRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wavesRitem->ObjCBIndex = objCBIndex;
	wavesRitem->Mat = mMaterials["water"].get();
	wavesRitem->Geo = mGeometries["waterGeo"].get();
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mWavesRitem = wavesRitem.get();

	mRitemLayer[(int)RenderLayer::Transparent].push_back(wavesRitem.get());
	//Build the land
	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	objCBIndex++;
	gridRitem->ObjCBIndex = objCBIndex;
	gridRitem->Mat = mMaterials["grass"].get();
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Transparent].push_back(gridRitem.get());
	


	//Build Ground & Grass
	Build_Render_Item("grid", XMMatrixScaling(1, 1, 1), XMMatrixTranslation(0.0f, 10.f, 0.0f), "stone", objCBIndex++);
	//Ground
	//Build_Render_Item("truncatedpyramid", XMMatrixScaling(70, 5.f, 70.f), XMMatrixTranslation(0, 2.5, 0), "grass", objCBIndex++);
	//Grass
	Build_Render_Item("truncatedcone", XMMatrixScaling(80, 2.f, 80.f), XMMatrixTranslation(0, 1, 0), "grass", objCBIndex++);
	
	//Build castle---------------------------------------------------
	BuildCastle(objCBIndex);
	//Finished Building Castle----------------------------------------------------------
	BuildMaze(objCBIndex);
	////// All the render items are opaque.
	
	// All the render items are opaque.
	//Our application will maintain lists of render items based on how they need to be
	//drawn; that is, render items that need different PSOs will be kept in different lists.Layer::AlphaTested].push_back(boxRitem.get());*/
	
	//Building Trees
	auto treeSpritesRitem = std::make_unique<RenderItem>();
	treeSpritesRitem->World = MathHelper::Identity4x4();
	treeSpritesRitem->ObjCBIndex = objCBIndex;
	treeSpritesRitem->Mat = mMaterials["treeSprites"].get();
	treeSpritesRitem->Geo = mGeometries["treeSpritesGeo"].get();
	//step2
	treeSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());

	//Building Lightning
	auto lightningSpritesRitem = std::make_unique<RenderItem>();
	lightningSpritesRitem->World = MathHelper::Identity4x4();
	lightningSpritesRitem->ObjCBIndex = objCBIndex;
	lightningSpritesRitem->Mat = mMaterials["rock1"].get();
	lightningSpritesRitem->Geo = mGeometries["lightningSpritesGeo"].get();
	//step2
	lightningSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	lightningSpritesRitem->IndexCount = lightningSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	lightningSpritesRitem->StartIndexLocation = lightningSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	lightningSpritesRitem->BaseVertexLocation = lightningSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(lightningSpritesRitem.get());
	mAllRitems.push_back(std::move(wavesRitem));
	mAllRitems.push_back(std::move(gridRitem));
	mAllRitems.push_back(std::move(treeSpritesRitem));
	mAllRitems.push_back(std::move(lightningSpritesRitem));
}

void CastleApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		//step3
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
		
		//Offset to the CBV in the descriptor heap for this object and for this frame resource.
		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CastleApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

float CastleApp::GetHillsHeight(float x, float z)const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 CastleApp::GetHillsNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}