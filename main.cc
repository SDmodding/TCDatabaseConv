#define THEORY_IMPL
#define THEORY_DUCKTAPE
#define THEORY_QSYMBOL_TABLE_INVENTORY
#include "theory/theory.hh"

#define XTag_TCDB						"TrueCrowdDataBase"
#define XTag_Definition					"Definition"
#define XTag_Entity						"Entity"
#define XTag_EntityComponent			"EntityComponent"
#define XTag_BoneUID					"BoneUID"
#define XTag_Tags						"Tags"
#define XTag_Tag						"Tag"
#define XTag_ComponentEntries			"ComponentEntries"
#define XTag_Component					"Component"
#define XTag_Resource					"Resource"
#define XTag_HighResolutionResource		"HighResolutionResource"
#define XTag_LOD						"LOD"
#define XTag_ModelPart					"ModelPart"
#define XTag_TextureSet					"TextureSet"
#define XTag_ColourTint					"ColourTint"
#define XTag_OverrideParam				"OverrideParam"

#define XAttr_Name						"name"
#define XAttr_NameUID					"nameUID"
#define XAttr_ResourceIndex				"resourceIndex"
#define XAttr_Required					"required"
#define XAttr_Type						"type"
#define XAttr_IsSkinned					"isSkinned"
#define XAttr_MorphType					"morphType"
#define XAttr_Sampler					"sampler"
#define XAttr_UID0						"uid0"
#define XAttr_UID1						"uid1"
#define XAttr_UID2						"uid2"

#include "converter.hh"
#include "scriber.hh"

////////////////////////////////////////////////////////////////////////////////////////////////
///		
///		XML Structure:
/// 
///		<TrueCrowdDataBase>
///			<Definition>
///				<Entity name="String">
///					<EntityComponent name="String" resourceIndex="Int32" required="Boolean">
///						<BoneUID>String</BoneUID>
///					</EntityComponent>
///				</Entity>
///				<Tags>
///					<Tag>String</Tag>
///				</Tags>
///			</Definition>
///			<ComponentEntries>
///				<Component name="String">
///					<Resource name="String" type="Int32">
///						<HighResolutionResource name="String"/>
///						<LOD>
///							<ModelPart name="String" isSkinned="Boolean" morphType="Int32"/>
///						</LOD>
///						<TextureSet name="String">
///							<ColourTint r="255" g="255" b="255"/>
///							<OverrideParam sampler="String" nameUID="UInt32" uid0="UInt32" uid1="UInt32" uid2="UInt32"/>
///							<HighResolutionResource name="String"/>
///						</TextureSet>
///						<Tag>String</Tag>
///					</Resource>
///				</Component>
///			</ComponentEntries>
///		</TrueCrowdDataBase>
/// 
////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///		Mapping to Resource:
///		
///		TrueCrowdDataBase::mDefinition -> <Definition>
///		|- mComponentCount -> <ComponentEntries> (number of <Component> children)
///		|- mComponents -> Build based on <ComponentEntries>/<Component> attributes
///		|- mNumTags -> <Tags> (number of <Tag> children)
///		|- mTagList -> <Tags>/<Tag> elements
/// 
///		TrueCrowdDataBase::mComponentEntries -> ComponentEntries
///		|- mNumEntries -> <Component> (number of <Resource> children)
///		|- mEntries -> <Component>/<Resource> elements
/// 
////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	auto GetArg = [&argc, &argv](const char* arg, bool isSet = 0) -> qString
	{
		for (int i = 0; argc > i; ++i)
		{
			if (UFG::qStringCompareInsensitive(argv[i], arg)) {
				continue;
			}

			if (isSet) {
				return argv[i];
			}

			int j = i + 1;
			if (argc > j) {
				return argv[j];
			}

			break;
		}

		return "";
	};

	qInit();

	const bool convert = !GetArg("-conv", 1).IsEmpty();
	const bool scribe = !GetArg("-scribe", 1).IsEmpty();
	auto qsymbols = GetArg("-qsymbols");
	auto filename = GetArg("-file");

	if (!convert && !scribe || filename.IsEmpty())
	{
		qPrintf("ERROR: Missing parameters.\n\n");
		qPrintf("Usage: %s [options]\n", argv[0]);
		qPrintf("\nOptions:\n");
		qPrintf("  %-25s %s\n", "-conv", "Convert TrueCrowdDataBase to XML.");
		qPrintf("  %-25s %s\n", "-scribe", "Scribe TrueCrowdDataBase in XML to binary file.");
		qPrintf("  %-25s %s\n", "-qsymbols <filename>", "QSymbol Table Resource to load.");
		qPrintf("  %-25s %s\n", "-file <filename>", "Specify the file for processing.");
		return 1;
	}

	/* Converter */

	if (convert)
	{
		if (qsymbols.IsEmpty() || !StreamResourceLoader::LoadResourceFile(qsymbols)) {
			qPrintf("WARN: QSymbols dictionary was not specified or failed to load. Symbols will be shown as hexadecimal strings.\n");
		}

		auto trueCrowdChunk = reinterpret_cast<qChunk*>(StreamFileWrapper::ReadEntireFile(filename));
		auto trueCrowdDB = static_cast<TrueCrowdDataBase*>(trueCrowdChunk->GetData());
		if (trueCrowdChunk->mUID != ChunkUID_TrueCrowdDataBase || trueCrowdDB->mTypeUID != RTypeUID_TrueCrowdDataBase)
		{
			qPrintf("ERROR: The input file is not a TrueCrowdDataBase resource.\n");
			return 1;
		}

		auto xmlFilename = filename.GetFilePathWithoutExtension() + ".xml";
		TCDatabaseConverter converter = { trueCrowdDB, xmlFilename };
		converter.Export();

		qPrintf("File has been exported to: %s\n", xmlFilename.mData);
		return 0;
	}

	/* Scriber */

	TCDatabaseScriber scriber = { filename };
	if (!scriber.mXML) {
		return 1;
	}

	if (!scriber.Build()) {
		return 1;
	}

	auto binFilename = filename.GetFilePathWithoutExtension() + ".bin";
	scriber.Export(binFilename);
	
	return 0;
}