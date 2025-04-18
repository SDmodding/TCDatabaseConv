#pragma once
#include <set>

using namespace UFG;

class TCDatabaseConverter
{
public:
	TrueCrowdDataBase* mDB;
	SimpleXML::XMLWriter* mXMLW;

	std::set<u32> mUnresolvedSymbols;

	TCDatabaseConverter(TrueCrowdDataBase* db, const char* filename) : mDB(db)
	{
		mXMLW = SimpleXML::XMLWriter::Create(filename, 0, 0x8000);
	}

	~TCDatabaseConverter()
	{
		SimpleXML::XMLWriter::Close(mXMLW);
		mXMLW = 0;
	}

	//------------------------------------
	//	Helpers
	//------------------------------------

	qString qSymbolStr(u32 uid)
	{
		if (auto str = qSymbolLookupStringFromSymbolTableResources(uid)) {
			return str;
		}

		mUnresolvedSymbols.insert(uid);

		return { "~0x%08X~", uid };
	}

	qString FormatUID(u32 uid) { return { "0x%X", uid }; }

	//------------------------------------
	//	Tags
	//------------------------------------

	void ExportTags(qSymbol* list, u32 count)
	{
		for (u32 i = 0; count > i; ++i)
		{
			mXMLW->BeginNode(XTag_Tag);
			mXMLW->AddValue(qSymbolStr(list[i]));
			mXMLW->EndNode(XTag_Tag);
		}
	}

	void ExportTags(const BitFlags128& bitFlags)
	{
		auto definition = &mDB->mDefinition;
		auto tag = definition->mTagList.Get();

		for (u32 i = 0; definition->mNumTags > i; ++i, ++tag)
		{
			if (!bitFlags.IsSet(i)) {
				continue;
			}

			mXMLW->BeginNode(XTag_Tag);
			mXMLW->AddValue(qSymbolStr(*tag));
			mXMLW->EndNode(XTag_Tag);
		}
	}

	//------------------------------------
	//	Definition
	//------------------------------------

	void ExportComponent(TrueCrowdDefinition::Component* component)
	{
		mXMLW->BeginNode(XTag_Component);
		mXMLW->AddValue(component->mName);
		mXMLW->EndNode(XTag_Component);
	}

	void ExportEntityComponent(TrueCrowdDefinition::Entity::EntityComponent* component)
	{
		mXMLW->BeginNode(XTag_EntityComponent);

		mXMLW->AddAttribute(XAttr_Name, qSymbolStr(component->mName));
		mXMLW->AddAttribute(XAttr_ResourceIndex, component->mResourceIndex);
		mXMLW->AddAttribute(XAttr_Required, component->mbRequired);

		for (u32 i = 0; component->mNumBoneUIDs > i; ++i)
		{
			mXMLW->BeginNode(XTag_BoneUID);
			mXMLW->AddValue(qSymbolStr(component->mBoneUID[i]));
			mXMLW->EndNode(XTag_BoneUID);
		}

		mXMLW->EndNode(XTag_EntityComponent);
	}

	void ExportEntity(TrueCrowdDefinition::Entity* entity)
	{
		mXMLW->BeginNode(XTag_Entity);

		mXMLW->AddAttribute(XAttr_Name, qSymbolStr(entity->mNameUID));

		for (u32 i = 0; entity->mComponentCount > i; ++i) {
			ExportEntityComponent(&entity->mComponents[i]);
		}

		mXMLW->EndNode(XTag_Entity);
	}


	void ExportDefinition(TrueCrowdDefinition* definition)
	{
		mXMLW->BeginNode(XTag_Definition);

		// Entites

		for (u32 i = 0; definition->mEntityCount > i; ++i) {
			ExportEntity(&definition->mEntities[i]);
		}

		// Tags

		mXMLW->BeginNode(XTag_Tags);

		ExportTags(definition->mTagList.Get(), definition->mNumTags);

		mXMLW->EndNode(XTag_Tags);

		mXMLW->EndNode(XTag_Definition);
	}

	//------------------------------------
	//	Resource
	//------------------------------------

	void ExportModelPart(TrueCrowdModelPart* modelPart)
	{
		mXMLW->BeginNode(XTag_ModelPart);

		mXMLW->AddAttribute(XAttr_Name, modelPart->mModelName.Get());
		mXMLW->AddAttribute(XAttr_IsSkinned, static_cast<u32>(modelPart->mIsSkinned));
		mXMLW->AddAttribute(XAttr_MorphType, static_cast<u32>(modelPart->mMorphType.mValue));

		mXMLW->EndNode(XTag_ModelPart);
	}

	void ExportLOD(TrueCrowdLOD* lod)
	{
		mXMLW->BeginNode(XTag_LOD);

		if (lod->mNumModelParts)
		{
			if (auto modelParts = lod->mModelParts.Get())
			{
				for (u32 i = 0; lod->mNumModelParts > i; ++i) {
					ExportModelPart(&modelParts[i]);
				}
			}
		}

		mXMLW->EndNode(XTag_LOD);
	}

	void ExportColourTint(qColour* col)
	{
		mXMLW->BeginNode(XTag_ColourTint);
		mXMLW->AddAttribute("r", static_cast<u32>(col->r * 255.f));
		mXMLW->AddAttribute("g", static_cast<u32>(col->g * 255.f));
		mXMLW->AddAttribute("b", static_cast<u32>(col->b * 255.f));
		mXMLW->EndNode(XTag_ColourTint);
	}

	void ExportTextureOverrideParam(TextureOverrideParams* param)
	{
		mXMLW->BeginNode(XTag_OverrideParam);

		mXMLW->AddAttribute(XAttr_Sampler, qSymbolStr(param->mSampler.mValue));
		mXMLW->AddAttribute(XAttr_NameUID, FormatUID(param->mTextureNameUID));

		mXMLW->AddAttribute(XAttr_UID0, FormatUID(param->mTextureOverrideUID[0]));
		mXMLW->AddAttribute(XAttr_UID1, FormatUID(param->mTextureOverrideUID[1]));
		mXMLW->AddAttribute(XAttr_UID2, FormatUID(param->mTextureOverrideUID[2]));

		mXMLW->EndNode(XTag_OverrideParam);
	}

	void ExportTexture(TrueCrowdTextureSet* textureSet)
	{
		mXMLW->BeginNode(XTag_TextureSet);

		mXMLW->AddAttribute(XAttr_Name, textureSet->mName.Get());

		if (auto colourTints = textureSet->mColourTints.Get())
		{
			for (u32 i = 0; textureSet->mNumColorTints > i; ++i) {
				ExportColourTint(&colourTints[i]);
			}
		}

		if (auto params = textureSet->mTextureOverrideParams.Get())
		{
			for (u32 i = 0; textureSet->mNumTextureOverrideParams > i; ++i) {
				ExportTextureOverrideParam(&params[i]);
			}
		}

		if (auto highResResource = textureSet->mHighResolutionResource.Get())
		{
			mXMLW->BeginNode(XTag_HighResolutionResource);
			mXMLW->AddAttribute(XAttr_Name, highResResource->mName.Get());
			mXMLW->EndNode(XTag_HighResolutionResource);
		}

		mXMLW->EndNode(XTag_TextureSet);
	}

	void ExportModel(TrueCrowdModel* model)
	{
		mXMLW->AddAttribute(XAttr_Name, model->mName.Get());
		mXMLW->AddAttribute(XAttr_Type, model->mType.mValue);

		if (auto highResResource = model->mHighResolutionResource.Get())
		{
			mXMLW->BeginNode(XTag_HighResolutionResource);
			mXMLW->AddAttribute(XAttr_Name, highResResource->mName.Get());
			mXMLW->EndNode(XTag_HighResolutionResource);
		}

		if (auto lodModels = model->mLODModel.Get())
		{
			for (u32 i = 0; model->mNumLODs > i; ++i) {
				ExportLOD(&lodModels[i]);
			}
		}

		if (auto textureSets = model->mTextureSets.Get())
		{
			for (u32 i = 0; model->mNumTextureSets > i; ++i) {
				ExportTexture(textureSets[i].Get());
			}
		}
	}

	void ExportResourceEntry(TrueCrowdDataBase::ResourceEntry* entry)
	{
		mXMLW->BeginNode(XTag_Resource);

		ExportModel(&entry->mResource);
		ExportTags(entry->mTagBitFlag);

		mXMLW->EndNode(XTag_Resource);
	}

	void ExportResourceEntries(TrueCrowdDataBase::ResourceEntry* entries, u32 count)
	{
		if (!entries || !count) {
			return;
		}

		for (u32 i = 0; count > i; ++i)
		{
			auto entry = &entries[i];
			ExportResourceEntry(entry);
		}
	}

	void ExportComponentEntries(TrueCrowdDataBase::ComponentEntries* entries, u32 count)
	{
		if (!entries || !count) {
			return;
		}

		for (u32 i = 0; count > i; ++i)
		{
			mXMLW->BeginNode(XTag_Component);

			mXMLW->AddAttribute(XAttr_Name, mDB->mDefinition.mComponents[i].mName);

			auto entry = &entries[i];
			ExportResourceEntries(entry->mEntries.Get(), entry->mNumEntries);

			mXMLW->EndNode(XTag_Component);
		}
	}

	void Export()
	{
		mXMLW->BeginNode(XTag_TCDB);

		ExportDefinition(&mDB->mDefinition);

		mXMLW->BeginNode(XTag_ComponentEntries);
		{
			ExportComponentEntries(mDB->mComponentEntries.Get(), mDB->mNumComponentEntries);
		}
		mXMLW->EndNode(XTag_ComponentEntries);

		mXMLW->EndNode(XTag_TCDB);

		if (!mUnresolvedSymbols.empty())
		{
			mXMLW->AddComment(" List of unresolved symbols ");

			for (auto uid : mUnresolvedSymbols)
			{
				qString str = { " 0x%X ", uid };
				mXMLW->AddComment(str);
			}
		}
	}
};