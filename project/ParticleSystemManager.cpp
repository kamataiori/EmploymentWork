#include "ParticleSystemManager.h"

#include <Logger.h>
#include <fstream>
#include <filesystem>
#include <json.hpp>

using json = nlohmann::json;

// ===============================================
// 注意：このファイルは b-1 の段階では空の実装のみ
// b-3 以降で ParticleManager から実装を移植する
// ===============================================

// ===== 生成・検索 =====

ParticleSystem* ParticleSystemManager::Create(const std::string& /*systemName*/)
{
	// TODO: b-3 で ParticleManager::CreateSystem の中身を移植
	return nullptr;
}

ParticleSystem* ParticleSystemManager::Find(const std::string& /*systemName*/)
{
	// TODO: b-3 で移植
	return nullptr;
}

const ParticleSystem* ParticleSystemManager::Find(const std::string& /*systemName*/) const
{
	// TODO: b-3 で移植
	return nullptr;
}

bool ParticleSystemManager::Rename(const std::string& /*oldName*/, const std::string& /*newName*/)
{
	// TODO: b-3 で移植
	return false;
}

void ParticleSystemManager::ClearAll()
{
	// TODO: b-3 で移植
}

// ===== 一覧 =====

std::vector<std::string> ParticleSystemManager::GetAllNames() const
{
	// TODO: b-3 で移植
	return {};
}

// ===== プリセット登録 =====

void ParticleSystemManager::RegisterPreset(const std::string& /*systemName*/,
	const std::string& /*presetName*/)
{
	// TODO: b-4 で移植
}

const std::vector<std::string>* ParticleSystemManager::GetPresets(const std::string& /*systemName*/) const
{
	// TODO: b-4 で移植
	return nullptr;
}

// ===== Emitter 登録 =====

void ParticleSystemManager::RegisterEmitter(
	const std::string& /*systemName*/,
	ParticleEmitterInstance* /*emitter*/,
	float /*startTime*/,
	float /*duration*/,
	bool /*autoPlay*/)
{
	// TODO: b-6 で実装
}

// ===== JSON =====

bool ParticleSystemManager::SaveToJson(const std::string& /*systemName*/,
	const std::string& /*directory*/)
{
	// TODO: b-5 で移植
	return false;
}

bool ParticleSystemManager::LoadFromJson(const std::string& /*systemName*/,
	const std::string& /*directory*/)
{
	// TODO: b-5 で移植
	return false;
}

void ParticleSystemManager::LoadAll(const std::string& /*directory*/)
{
	// TODO: b-5 で移植
}

// ===== Update 駆動 =====

void ParticleSystemManager::UpdateAll(float dt)
{
	for (auto& system : systems_) {
		if (system) {
			system->Update(dt);
		}
	}
}