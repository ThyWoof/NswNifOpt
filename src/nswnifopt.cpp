/*
nswnifopt: remove editor markers and optimize for SSE.
Copyright (c) 2019 Thy Woof

Optimize NIF function from https://github.com/ousnius/BodySlide-and-Outfit-Studio

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "..\lib\cxxopts\cxxopts.hpp"
#include "..\lib\NIF\NifFile.h"

bool RemoveEditorMarker(NifFile& nif)
{
	bool changed = false;

	for (auto &shape : nif.GetShapes()) {
		std::string shapeName = shape->GetName();

		if (shapeName == "EditorMarker") {
			printf("Found an editor marker. Deleting...\n");
			nif.DeleteShape(shape);
			changed = true;
		}
	}
	
	return changed;
}

void OptimizeNIF(NifFile& nif, bool headParts = false, bool removeParallax = true, bool calcBounds = true)
{
	NiHeader& hdr = nif.GetHeader();
	NiVersion& version = hdr.GetVersion();

	version.SetFile(V20_2_0_7);
	version.SetUser(12);
	version.SetStream(100);

	for (auto &shape : nif.GetShapes()) {
		std::string shapeName = shape->GetName();

		auto geomData = hdr.GetBlock<NiGeometryData>(shape->GetDataRef());
		if (geomData) {
			bool removeVertexColors = true;
			bool hasTangents = geomData->HasTangents();
			std::vector<Vector3>* vertices = &geomData->vertices;
			std::vector<Vector3>* normals = &geomData->normals;
			const std::vector<Color4>& colors = geomData->vertexColors;
			std::vector<Vector2>* uvs = nullptr;
			if (!geomData->uvSets.empty())
				uvs = &geomData->uvSets[0];

			std::vector<Triangle> triangles;
			geomData->GetTriangles(triangles);

			// Only remove vertex colors if all are 0xFFFFFFFF
			Color4 white(1.0f, 1.0f, 1.0f, 1.0f);
			for (auto &c : colors) {
				if (white != c) {
					removeVertexColors = false;
					break;
				}
			}

			bool headPartEyes = false;
			NiShader* shader = nif.GetShader(shape);
			if (shader) {
				auto bslsp = dynamic_cast<BSLightingShaderProperty*>(shader);
				if (bslsp) {
					// Remember eyes flag for later
					if ((bslsp->shaderFlags1 & (1 << 17)) != 0)
						headPartEyes = true;

					// No normals and tangents with model space maps
					if ((bslsp->shaderFlags1 & (1 << 12)) != 0) {
						normals = nullptr;
					}

					// Check tree anim flag
					if ((bslsp->shaderFlags2 & (1 << 29)) != 0)
						removeVertexColors = false;

					// Disable flag if vertex colors were removed
					if (removeVertexColors)
						bslsp->shaderFlags2 &= ~(1 << 5);

					if (removeParallax) {
						if (bslsp->GetShaderType() == BSLSP_PARALLAX) {
							// Change type from parallax to default
							bslsp->SetShaderType(BSLSP_DEFAULT);

							// Remove parallax flag
							bslsp->shaderFlags1 &= ~(1 << 11);

							// Remove parallax texture from set
							auto textureSet = hdr.GetBlock<BSShaderTextureSet>(shader->GetTextureSetRef());
							if (textureSet && textureSet->numTextures >= 4)
								textureSet->textures[3].Clear();
						}
					}
				}

				auto bsesp = dynamic_cast<BSEffectShaderProperty*>(shader);
				if (bsesp) {
					// Remember eyes flag for later
					if ((bsesp->shaderFlags1 & (1 << 17)) != 0)
						headPartEyes = true;

					// Check tree anim flag
					if ((bsesp->shaderFlags2 & (1 << 29)) != 0)
						removeVertexColors = false;

					// Disable flag if vertex colors were removed
					if (removeVertexColors)
						bsesp->shaderFlags2 &= ~(1 << 5);
				}
			}

			BSTriShape* bsOptShape = nullptr;
			auto bsSegmentShape = dynamic_cast<BSSegmentedTriShape*>(shape);
			if (bsSegmentShape) {
				bsOptShape = new BSSubIndexTriShape();
			}
			else {
				if (headParts)
					bsOptShape = new BSDynamicTriShape();
				else
					bsOptShape = new BSTriShape();
			}

			bsOptShape->SetName(shape->GetName());
			bsOptShape->SetControllerRef(shape->GetControllerRef());
			bsOptShape->SetSkinInstanceRef(shape->GetSkinInstanceRef());
			bsOptShape->SetShaderPropertyRef(shape->GetShaderPropertyRef());
			bsOptShape->SetAlphaPropertyRef(shape->GetAlphaPropertyRef());
			bsOptShape->SetCollisionRef(shape->GetCollisionRef());
			bsOptShape->GetProperties() = shape->GetProperties();
			bsOptShape->GetExtraData() = shape->GetExtraData();

			bsOptShape->transform = shape->transform;

			bsOptShape->Create(vertices, &triangles, uvs, normals);
			bsOptShape->flags = shape->flags;

			// Move segments to new shape
			if (bsSegmentShape) {
				auto bsSITS = dynamic_cast<BSSubIndexTriShape*>(bsOptShape);
				bsSITS->numSegments = bsSegmentShape->numSegments;
				bsSITS->segments = bsSegmentShape->segments;
			}

			// Restore old bounds for static meshes or when calc bounds is off
			if (!shape->IsSkinned() || !calcBounds)
				bsOptShape->SetBounds(geomData->GetBounds());

			// Vertex Colors
			if (bsOptShape->GetNumVertices() > 0) {
				if (!removeVertexColors && colors.size() > 0) {
					bsOptShape->SetVertexColors(true);
					for (int i = 0; i < bsOptShape->GetNumVertices(); i++) {
						auto& vertex = bsOptShape->vertData[i];

						float f = std::max(0.0f, std::min(1.0f, colors[i].r));
						vertex.colorData[0] = (byte)std::floor(f == 1.0f ? 255 : f * 256.0);

						f = std::max(0.0f, std::min(1.0f, colors[i].g));
						vertex.colorData[1] = (byte)std::floor(f == 1.0f ? 255 : f * 256.0);

						f = std::max(0.0f, std::min(1.0f, colors[i].b));
						vertex.colorData[2] = (byte)std::floor(f == 1.0f ? 255 : f * 256.0);

						f = std::max(0.0f, std::min(1.0f, colors[i].a));
						vertex.colorData[3] = (byte)std::floor(f == 1.0f ? 255 : f * 256.0);
					}
				}

				// Find NiOptimizeKeep string
				for (auto& extraData : bsOptShape->GetExtraData()) {
					auto stringData = hdr.GetBlock<NiStringExtraData>(extraData.GetIndex());
					if (stringData) {
						if (stringData->GetStringData().find("NiOptimizeKeep") != std::string::npos) {
							bsOptShape->particleDataSize = bsOptShape->GetNumVertices() * 6 + triangles.size() * 3;
							bsOptShape->particleVerts = *vertices;

							bsOptShape->particleNorms.resize(vertices->size(), Vector3(1.0f, 0.0f, 0.0f));
							if (normals && normals->size() == vertices->size())
								bsOptShape->particleNorms = *normals;

							bsOptShape->particleTris = triangles;
						}
					}
				}

				// Skinning and partitions
				if (shape->IsSkinned()) {
					bsOptShape->SetSkinned(true);

					auto skinInst = hdr.GetBlock<NiSkinInstance>(shape->GetSkinInstanceRef());
					if (skinInst) {
						auto skinPart = hdr.GetBlock<NiSkinPartition>(skinInst->GetSkinPartitionRef());
						if (skinPart) {
							bool triangulated = nif.TriangulatePartitions(shape);

							for (int partID = 0; partID < skinPart->numPartitions; partID++) {
								NiSkinPartition::PartitionBlock& part = skinPart->partitions[partID];

								for (int i = 0; i < part.numVertices; i++) {
									const ushort v = part.vertexMap[i];

									if (bsOptShape->vertData.size() > v) {
										auto& vertex = bsOptShape->vertData[v];

										if (part.hasVertexWeights) {
											auto& weights = part.vertexWeights[i];
											vertex.weights[0] = weights.w1;
											vertex.weights[1] = weights.w2;
											vertex.weights[2] = weights.w3;
											vertex.weights[3] = weights.w4;
										}

										if (part.hasBoneIndices) {
											auto& boneIndices = part.boneIndices[i];
											vertex.weightBones[0] = part.bones[boneIndices.i1];
											vertex.weightBones[1] = part.bones[boneIndices.i2];
											vertex.weightBones[2] = part.bones[boneIndices.i3];
											vertex.weightBones[3] = part.bones[boneIndices.i4];
										}
									}
								}

								std::vector<Triangle> realTris(part.numTriangles);
								for (int i = 0; i < part.numTriangles; i++) {
									auto& partTri = part.triangles[i];

									// Find the actual tri index from the partition tri index
									Triangle tri;
									tri.p1 = part.vertexMap[partTri.p1];
									tri.p2 = part.vertexMap[partTri.p2];
									tri.p3 = part.vertexMap[partTri.p3];

									tri.rot();
									realTris[i] = tri;
								}

								part.triangles = realTris;
								part.trueTriangles = realTris;
							}
						}
					}
				}
				else
					bsOptShape->SetSkinned(false);
			}
			else
				bsOptShape->SetVertices(false);

			// Enable eye data flag
			if (!bsSegmentShape) {
				if (headParts) {
					if (headPartEyes)
						bsOptShape->SetEyeData(true);
				}
			}

			hdr.ReplaceBlock(nif.GetBlockID(shape), bsOptShape);
			nif.UpdateSkinPartitions(bsOptShape);
		}
	}

	nif.DeleteUnreferencedBlocks();

	// For files without a root node, remove the leftover data blocks anyway
	hdr.DeleteBlockByType("NiTriStripsData", true);
	hdr.DeleteBlockByType("NiTriShapeData", true);

	return;
}

inline bool isFileValid(const std::string& name) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

int main(int argc, char* argv[], char* const envp[])
{

	std::string i_filename;
	std::string o_filename;
	bool flagRemoveEditorMarker = false;
	bool flagOptimizeNIF = false;
	bool flagIsHeadPart = false;
	bool flagRemoveParallax = true;
	bool flagCalcBounds = true;

	cxxopts::Options options(argv[0], "Zappastuff's Nintendo Switch Skyrim NIF optimizer");

	try
	{
		options
			.positional_help("[optional args]")
      		.show_positional_help()
			.add_options()
				("remove-editor-marker", "Remove Editor Marker", cxxopts::value<bool>(flagRemoveEditorMarker))
				("optimize-nif", "optimize NIFs", cxxopts::value<bool>(flagOptimizeNIF))				
				("is-head-part", "NIF is a head part", cxxopts::value<bool>(flagIsHeadPart))
				("i,input", "input file", cxxopts::value<std::string>())
				("o,output", "ouptput file", cxxopts::value<std::string>());

		auto result = options.parse(argc, argv);

	    if (!result.count("input") || !result.count("output"))
    	{
      		std::cout << options.help() << std::endl;
      		exit(1);
    	}

		if (!flagRemoveEditorMarker && !flagOptimizeNIF)
		{
      		std::cout << "nothing to do. aborting.";
      		exit(1);
		}

		i_filename = result["input"].as<std::string>();
		o_filename = result["output"].as<std::string>();

		if (!isFileValid(i_filename)) {
			std::cout << "input file doesn't exist. aborting.";
			exit(1);
		}
  	}
	catch (const cxxopts::OptionException& e) 
	{
    	std::cout << "error parsing options: " << e.what() << std::endl << std::endl << options.help() << std::endl;
    	exit(1);
  	}

	NifFile nif;	
	nif.Load(i_filename);

	NiHeader& hdr = nif.GetHeader();
	NiVersion& version = hdr.GetVersion();
	if (version.File() != 0x14020007 || version.User() != 12) {
		printf("NiVersion file: 0x%08x user: %u stream: %u not supported\n", version.File(), version.User(), version.Stream());
		exit(1);
	}

	if (flagRemoveEditorMarker)
		RemoveEditorMarker(nif);

	if (flagOptimizeNIF)
		OptimizeNIF(nif, flagIsHeadPart, flagRemoveParallax, flagCalcBounds);

	nif.Save(o_filename);
	return 0;
}