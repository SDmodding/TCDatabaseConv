#pragma once
#include <map>

using namespace UFG;

class TCDatabaseScriber
{
public:
	TrueCrowdDataBase* mDB;
	u32 mByteSize = 0;

	SimpleXML::XMLDocument* mXML;

	// Resource

	TrueCrowdDataBase::ResourceEntry* mResourceEntry;

	TrueCrowdLOD* mLOD;
	TrueCrowdModelPart* mModelPart;

	qOffset64<TrueCrowdTextureSet*>* mTextureSetArray;
	TrueCrowdTextureSet* mTextureSet;

	qColour* mColourTints;
	TextureOverrideParams* mTextureOverrideParams;

	char* mStrBuffer;
	u32 mStrLen;

	u32 mComponentTypeSymbolUC = 0;

	std::map<u32, const char*> mSymbols;

	struct qResourceOffsetFix
	{
		qOffset64<TrueCrowdResource*>* mOffset;
		const char* mName;
		bool mIsTextureSet;
	};

	std::vector<qResourceOffsetFix> mTrueCrowdResourceOffsetFixes;

	TCDatabaseScriber(const qString& filename) : mDB(0)
	{
		mXML = SimpleXML::XMLDocument::Open(filename);
	}

	~TCDatabaseScriber()
	{
		qDelete(mXML);
		mXML = 0;
	}

	//------------------------------------
	//	Helpers
	//------------------------------------

	u32 CreateSymbol(const char* str, bool uppercase)
	{
		if (*str == '~') {
			return strtoul(&str[1], 0, 16);
		}

		u32 sym = (uppercase ? qStringHashUpper32(str) : qStringHash32(str));
		mSymbols[sym] = str;
		return sym;
	}

	const char* GetTypePath(int type)
	{
		switch (type)
		{
		default:
			return 0;
		case TrueCrowdResource::Character:
			return "Characters_New";
		case TrueCrowdResource::Vehicle:
			return "Vehicles_New";
		case TrueCrowdResource::Prop:
			return "Props_New";
		}
	}

	TrueCrowdResource* FindCrowdResource(const char* name, bool isTextureSet)
	{
		auto componentEntry = mDB->mComponentEntries.Get();
		for (u32 i = 0; mDB->mNumComponentEntries > i; ++i, ++componentEntry)
		{
			auto resourceEntry = componentEntry->mEntries.Get();
			for (u32 j = 0; componentEntry->mNumEntries > j; ++j, ++resourceEntry)
			{
				if (isTextureSet)
				{
					auto textureSets = resourceEntry->mResource.mTextureSets.Get();
					for (u32 k = 0; resourceEntry->mResource.mNumTextureSets > k; ++k, ++textureSets)
					{
						auto textureSet = textureSets->Get();
						auto textureSetName = textureSet->mName.Get();
						if (!qStringCompare(textureSetName, name)) {
							return textureSet;
						}
					}
					continue;
				}

				auto resourceName = resourceEntry->mResource.mName.Get();
				if (!qStringCompare(resourceName, name)) {
					return &resourceEntry->mResource;
				}
			}
		}

		return 0;
	}

	//------------------------------------
	//	Build
	//------------------------------------

	void BuildResource()
	{
		const char* name = "TrueCrowdDB";
		mDB->SetDebugName(name);
		mDB->mNode.mUID = qStringHash32(name);
		mDB->mTypeUID = RTypeUID_TrueCrowdDataBase;
	}

	const char* AppendStringBuffer(const char* str)
	{
		int len = qStringLength(str);
		char* buf = &mStrBuffer[mStrLen];
		mStrLen += len + 1;

		qMemCopy(buf, str, len);
		buf[len] = 0;

		return buf;
	}

	void BuildResource(TrueCrowdResource* resource, const char* name, int type)
	{
		const char* typePath = GetTypePath(type);

		resource->mName.Set(AppendStringBuffer(name));
		resource->mType = type;
		
		qString buf;
		buf.Format("Data\\%s\\%s", typePath, name);
		resource->mPathSymbol = buf.GetStringHash32();

		buf.Format("TrueCrowd-%s-%s", typePath, name);
		resource->mPropSetName = buf.GetStringHash32();
	}

	void BuildTagBitFlags(BitFlags128* bitFlags, SimpleXML::XMLNode* node)
	{
		auto definition = &mDB->mDefinition;
		auto tags = definition->mTagList.Get();

		for (auto tag = mXML->GetChildNode(XTag_Tag, node); tag; tag = mXML->GetNode(XTag_Tag, tag))
		{
			auto tagStr = tag->GetValue();
			const u32 tagUID = CreateSymbol(tagStr, 0);
			for (u32 i = 0; definition->mNumTags > i; ++i)
			{
				if (tags[i] == tagUID)
				{
					bitFlags->Set(i);
					break;
				}
			}
		}
	}

	void BuildModelPart(TrueCrowdModelPart* modelPart, SimpleXML::XMLNode* node)
	{
		const char* name = AppendStringBuffer(node->GetAttribute(XAttr_Name));
		modelPart->mModelName.Set(name);
		modelPart->mModelNameHash = CreateSymbol(name, 1);
		modelPart->mIsSkinned = node->GetAttribute(XAttr_IsSkinned, 0);
		modelPart->mMorphType = node->GetAttribute(XAttr_MorphType, 0);
	}

	void BuildLODModel(TrueCrowdLOD* lod, SimpleXML::XMLNode* node)
	{
		auto modelPart = mXML->GetChildNode(XTag_ModelPart, node);
		if (!modelPart) {
			return;
		}

		lod->mModelParts.Set(mModelPart);

		for (; modelPart; modelPart = mXML->GetNode(XTag_ModelPart, modelPart))
		{
			BuildModelPart(mModelPart++, modelPart);
			++lod->mNumModelParts;
		}
	}

	void BuildTextureSet(TrueCrowdTextureSet* textureSet, int type, SimpleXML::XMLNode* node)
	{
		BuildResource(textureSet, node->GetAttribute(XAttr_Name), type);

		auto colourTint = mXML->GetChildNode(XTag_ColourTint, node);
		if (colourTint)
		{
			textureSet->mColourTints.Set(mColourTints);

			for (; colourTint; colourTint = mXML->GetNode(XTag_ColourTint, colourTint))
			{
				auto tint = mColourTints++;

				tint->r = static_cast<f32>(colourTint->GetAttribute("r", 0) / 255.f);
				tint->g = static_cast<f32>(colourTint->GetAttribute("g", 0) / 255.f);
				tint->b = static_cast<f32>(colourTint->GetAttribute("b", 0) / 255.f);

				++textureSet->mNumColorTints;
			}
		}

		auto overrideParam = mXML->GetChildNode(XTag_OverrideParam, node);
		if (overrideParam)
		{
			textureSet->mTextureOverrideParams.Set(mTextureOverrideParams);

			for (; overrideParam; overrideParam = mXML->GetNode(XTag_OverrideParam, overrideParam))
			{
				auto param = mTextureOverrideParams++;

				param->mSampler = CreateSymbol(overrideParam->GetAttribute(XAttr_Sampler), 0);
				param->mTextureNameUID = overrideParam->GetAttribute(XAttr_NameUID, 0u);
				param->mTextureOverrideUID[0] = overrideParam->GetAttribute(XAttr_UID0, 0u);
				param->mTextureOverrideUID[1] = overrideParam->GetAttribute(XAttr_UID1, 0u);
				param->mTextureOverrideUID[2] = overrideParam->GetAttribute(XAttr_UID2, 0u);

				++textureSet->mNumTextureOverrideParams;
			}
		}

		if (auto highResResource = mXML->GetChildNode(XTag_HighResolutionResource, node)) {
			mTrueCrowdResourceOffsetFixes.push_back({ &textureSet->mHighResolutionResource, highResResource->GetAttribute(XAttr_Name), 1 });
		}
	}

	void BuildResourceEntry(TrueCrowdDataBase::ResourceEntry* entry, SimpleXML::XMLNode* node)
	{
		auto model = &entry->mResource;
		const char* name = node->GetAttribute(XAttr_Name);
		int type = node->GetAttribute(XAttr_Type, TrueCrowdResource::Invalid);

		if (type == TrueCrowdResource::Invalid)	{
			qPrintf("WARN: Resource %s has an invalid type specified.\n", name);
		}

		BuildTagBitFlags(&entry->mTagBitFlag, node);
		BuildResource(model, name, type);

		model->mComponentTypeSymbolUC = mComponentTypeSymbolUC;

		if (auto highResResource = mXML->GetChildNode(XTag_HighResolutionResource, node)) {
			mTrueCrowdResourceOffsetFixes.push_back({ &model->mHighResolutionResource, highResResource->GetAttribute(XAttr_Name), 0 });
		}

		auto lod = mXML->GetChildNode(XTag_LOD, node);
		if (lod)
		{
			model->mLODModel.Set(mLOD);

			for (; lod; lod = mXML->GetNode(XTag_LOD, lod))
			{
				BuildLODModel(mLOD++, lod);
				++model->mNumLODs;
			}
		}

		auto textureSet = mXML->GetChildNode(XTag_TextureSet, node);
		if (textureSet)
		{
			model->mTextureSets.Set(mTextureSetArray);

			for (; textureSet; textureSet = mXML->GetNode(XTag_TextureSet, textureSet))
			{
				BuildTextureSet(mTextureSet, type, textureSet);
				mTextureSetArray->Set(mTextureSet++);
				++mTextureSetArray;

				++model->mNumTextureSets;	
			}
		}
	}

	void BuildComponent(TrueCrowdDefinition::Component* component, TrueCrowdDataBase::ComponentEntries* entry, SimpleXML::XMLNode* node)
	{
		const char* name = node->GetAttribute(XAttr_Name);
		strcpy(component->mName, name);
		component->mNameUID = CreateSymbol(name, 1);

		mComponentTypeSymbolUC = component->mNameUID;

		auto resource = mXML->GetChildNode(XTag_Resource, node);
		if (!resource) {
			return;
		}

		entry->mEntries.Set(mResourceEntry);

		for (; resource; resource = mXML->GetNode(XTag_Resource, resource))
		{
			BuildResourceEntry(mResourceEntry++, resource);
			++entry->mNumEntries;
		}
	}

	void BuildEntity(TrueCrowdDefinition::Entity* entity, SimpleXML::XMLNode* node)
	{
		const char* name = node->GetAttribute(XAttr_Name);
		entity->mNameUID = CreateSymbol(name, 1);

		for (auto component = mXML->GetChildNode(XTag_EntityComponent, node); component; component = mXML->GetNode(XTag_EntityComponent, component))
		{
			const char* componentname = component->GetAttribute(XAttr_Name);

			auto entitycomponent = &entity->mComponents[entity->mComponentCount++];
			entitycomponent->mName = CreateSymbol(componentname, 0);
			entitycomponent->mResourceIndex = component->GetAttribute(XAttr_ResourceIndex, 0);
			entitycomponent->mbRequired = component->GetAttribute(XAttr_Required, 0);

			for (auto boneuid = mXML->GetChildNode(XTag_BoneUID, component); boneuid; boneuid = mXML->GetNode(XTag_BoneUID, boneuid))
			{
				const char* bonename = boneuid->GetValue();
				entitycomponent->mBoneUID[entitycomponent->mNumBoneUIDs++] = CreateSymbol(bonename, 1);
			}

			if (entitycomponent->mbRequired) {
				++entity->mRequiredComponentCount;
			}
		}
	}

	bool BuildSchema()
	{
		auto schema = Illusion::GetSchema(); 
		
		schema->Init();
		schema->Add("TrueCrowdDatabase", &mDB);

		auto xDB = mXML->GetChildNode(XTag_TCDB);
		if (!xDB)
		{
			qPrintf("ERROR: Required XML tag <%s> is missing.\n", XTag_TCDB);
			return 0;
		}

		auto xDefinition = mXML->GetChildNode(XTag_Definition, xDB);
		if (!xDefinition)
		{
			qPrintf("ERROR: Required XML tag <%s> is missing inside <%s>.\n", XTag_Definition, XTag_TCDB);
			return 0;
		}

		auto xComponentEntries = mXML->GetChildNode(XTag_ComponentEntries, xDB);
		if (!xComponentEntries)
		{
			qPrintf("ERROR: Required XML tag <%s> is missing inside <%s>.\n", XTag_ComponentEntries, XTag_TCDB);
			return 0;
		}

		auto xTags = mXML->GetChildNode(XTag_Tags, xDefinition);
		const u32 numTags = xTags->GetChildCount();
		if (numTags) {
			schema->AddArray<qSymbol>("TagList", numTags, 0, &mDB->mDefinition.mTagList);
		}
		else {
			qPrintf("WARN: Missing XML tag <%s> inside <%s>. Was this intended?", XTag_Tags, XTag_Definition);
		}

		const u32 numComponentEntries = xComponentEntries->GetChildCount();
		if (numComponentEntries) {
			schema->AddArray<TrueCrowdDataBase::ComponentEntries>("ComponentEntries", numComponentEntries, 0, &mDB->mComponentEntries);
		}

		/* Precalculate required stuff. */

		u32 numResourceEntries = 0;
		u32 numLODs = 0;
		u32 numModelParts = 0;
		u32 numTextureSets = 0;
		u32 numColourTints = 0;
		u32 numTextureOverrideParams = 0;

		u32 strsSize = 0;

		for (auto component = mXML->GetChildNode(XTag_Component, xComponentEntries); component; component = mXML->GetNode(XTag_Component, component))
		{
			numResourceEntries += component->GetChildCount();

			for (auto resource = mXML->GetChildNode(XTag_Resource, component); resource; resource = mXML->GetNode(XTag_Resource, resource))
			{
				strsSize += qStringLength(resource->GetAttribute(XAttr_Name)) + 1;

				for (auto lod = mXML->GetChildNode(XTag_LOD, resource); lod; lod = mXML->GetNode(XTag_LOD, lod))
				{
					++numLODs;

					for (auto modelPart = mXML->GetChildNode(XTag_ModelPart, lod); modelPart; modelPart = mXML->GetNode(XTag_ModelPart, modelPart))
					{
						strsSize += qStringLength(modelPart->GetAttribute(XAttr_Name)) + 1;
						++numModelParts;
					}
				}

				for (auto textureSet = mXML->GetChildNode(XTag_TextureSet, resource); textureSet; textureSet = mXML->GetNode(XTag_TextureSet, textureSet))
				{
					strsSize += qStringLength(textureSet->GetAttribute(XAttr_Name)) + 1;
					++numTextureSets;

					for (auto colourTint = mXML->GetChildNode(XTag_ColourTint, textureSet); colourTint; colourTint = mXML->GetNode(XTag_ColourTint, colourTint)) {
						++numColourTints;
					}

					for (auto overrideParam = mXML->GetChildNode(XTag_OverrideParam, textureSet); overrideParam; overrideParam = mXML->GetNode(XTag_OverrideParam, overrideParam)) {
						++numTextureOverrideParams;
					}
				}
			}
		}

		schema->AddArray("ResourceEntries", numResourceEntries, &mResourceEntry);
		schema->AddArray("LODs", numLODs, &mLOD);
		schema->AddArray("ModelParts", numModelParts, &mModelPart);
		schema->AddArray("TextureSetArray", numTextureSets, &mTextureSetArray);
		schema->AddArray("TextureSets", numTextureSets, &mTextureSet);
		schema->AddArray("ColourTints", numColourTints, &mColourTints);
		schema->AddArray("TextureOverrideParams", numTextureOverrideParams, &mTextureOverrideParams);
		schema->Add("StringBuffer", strsSize, (void**)&mStrBuffer);

		schema->Allocate();

		mByteSize = static_cast<u32>(schema->mCurrSize);
		return 1;
	}

	bool Build()
	{
		if (!BuildSchema()) {
			return 0;
		}

		BuildResource();

		auto definition = &mDB->mDefinition;

		auto xDB = mXML->GetChildNode(XTag_TCDB);
		auto xDefinition = mXML->GetChildNode(XTag_Definition, xDB);
		auto xComponentEntries = mXML->GetChildNode(XTag_ComponentEntries, xDB);

		for (auto entity = mXML->GetChildNode(XTag_Entity, xDefinition); entity; entity = mXML->GetNode(XTag_Entity, entity)) {
			BuildEntity(&definition->mEntities[definition->mEntityCount++], entity);
		}

		auto tagList = definition->mTagList.Get();

		auto xTags = mXML->GetChildNode(XTag_Tags, xDefinition);
		for (auto tag = mXML->GetChildNode(XTag_Tag, xTags); tag; tag = mXML->GetNode(XTag_Tag, tag)) {
			tagList[definition->mNumTags++] = CreateSymbol(tag->GetValue(), 0);
		}

		auto componentEntries = mDB->mComponentEntries.Get();
		for (auto component = mXML->GetChildNode(XTag_Component, xComponentEntries); component; component = mXML->GetNode(XTag_Component, component)) {
			BuildComponent(&definition->mComponents[definition->mComponentCount++], &componentEntries[mDB->mNumComponentEntries++], component);
		}

		for (auto& resourceFix : mTrueCrowdResourceOffsetFixes)
		{
			auto resource = FindCrowdResource(resourceFix.mName, resourceFix.mIsTextureSet);
			if (!resource)
			{
				qPrintf("ERROR: Failed to fix resource offset for %s (%s)\n", resourceFix.mName, (resourceFix.mIsTextureSet ? "TextureSet" : "Model"));
				return 0;
			}

			resourceFix.mOffset->Set(resource);
		}

		return 1;
	}

	void Export(const char* filename)
	{
		qString qSymbolsFilename = filename;
		qSymbolsFilename = qSymbolsFilename.GetFilePathWithoutExtension() + "_qsymbols.txt";

		if (auto f = qOpen(qSymbolsFilename, QACCESS_WRITE))
		{
			qString buf;

			for (auto& sym : mSymbols)
			{
				buf.Format("0x%08X %s\n", sym.first, sym.second);
				qWriteString(f, buf, buf.Length());
			}

			qClose(f);
			qPrintf("QSymbols has been exported to: %s\n", qSymbolsFilename.mData);
		}

		qChunkFileBuilder chunkBuilder;
		chunkBuilder.CreateBuilder("PC64", filename, 0, 0);

		chunkBuilder.BeginChunk(ChunkUID_TrueCrowdDataBase, "TrueCrowdDB", 1);
		chunkBuilder.Write(mDB, mByteSize);
		chunkBuilder.EndChunk(ChunkUID_TrueCrowdDataBase);

		chunkBuilder.CloseBuilder(0, true);

		qPrintf("File has been exported to: %s\n", filename);
	}
};