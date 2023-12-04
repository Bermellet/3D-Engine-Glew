#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

#include "Mesh.h"
#include "MathGeoLib.h"
#include <GL/glew.h>
#include <SDL_assert.h>

Mesh::Mesh()
{
	vao = vbo = ebo = 0;
	vertexCount = indexCount = 0;
	interleavedVertices = false;
}

Mesh::~Mesh()
{

}

void Mesh::Load(tinygltf::Model model, tinygltf::Mesh mesh, tinygltf::Primitive primitive)
{
	const auto& itPos = primitive.attributes.find("POSITION");
	if (itPos != primitive.attributes.end())
	{
		const tinygltf::Accessor& posAcc = model.accessors[itPos->second];
		SDL_assert(posAcc.type == TINYGLTF_TYPE_VEC3);
		SDL_assert(posAcc.componentType == GL_FLOAT);
		const tinygltf::BufferView& posView = model.bufferViews[posAcc.bufferView];
		const tinygltf::Buffer& posBuffer = model.buffers[posView.buffer];
		const unsigned char* bufferPos = &(posBuffer.data[posAcc.byteOffset + posView.byteOffset]);
		vertexCount = posAcc.count;

		// No byte stride
		glGenBuffers(1, &vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * posAcc.count, bufferPos, GL_STATIC_DRAW);

		// Using byte stride
		/*glGenBuffers(1, &vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * posAcc.count, nullptr, GL_STATIC_DRAW);
		float3* ptr = reinterpret_cast<float3*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
		for (size_t i = 0; i < posAcc.count; ++i)
		{
			ptr[i] = *reinterpret_cast<const float3*>(bufferPos);
			bufferPos += posView.byteStride;
		}*/
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	if (primitive.indices >= 0)
	{
		const tinygltf::Accessor& indAcc = model.accessors[primitive.indices];
		const tinygltf::BufferView& indView = model.bufferViews[indAcc.bufferView];
		indexCount = indAcc.count;
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indAcc.count, nullptr, GL_STATIC_DRAW);
		unsigned int* ptr = reinterpret_cast<unsigned int*>(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));
		const unsigned char* buffer = &(model.buffers[indView.buffer].data[indAcc.byteOffset +
			indView.byteOffset]);
		if (indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)
		{
			const uint32_t* bufferInd = reinterpret_cast<const uint32_t*>(buffer);
			for (uint32_t i = 0; i < indexCount; ++i) ptr[i] = bufferInd[i];
		}
		if (indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT)
		{
			const uint_least16_t* bufferInd = reinterpret_cast<const uint_least16_t*>(buffer);
			for (uint_least16_t i = 0; i < indexCount; ++i) ptr[i] = bufferInd[i];
		}
		if (indAcc.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)
		{
			const uint8_t* bufferInd = reinterpret_cast<const uint8_t*>(buffer);
			for (uint8_t i = 0; i < indexCount; ++i) ptr[i] = bufferInd[i];
		}
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	}

	CreateVAO(interleavedVertices);
	// TODO: material load and save materialIndex
}

void Mesh::RenderInterleaved(const unsigned programId) const
{
	glUseProgram(programId);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3 + sizeof(float) * 2, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3 + sizeof(float) * 2, (void*)(sizeof(float) * 3));
	glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}

void Mesh::RenderSeparated(const unsigned programId) const
{
	glUseProgram(programId);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * vertexCount));
	glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}
/*
void Mesh::RenderEBO()
{
	glUseProgram(program_id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * vertexCount));
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
}
*/

void Mesh::CreateVAO(bool interleaved)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glEnableVertexAttribArray(0);
	if (interleaved) {
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3 + sizeof(float) * 2, (void*)0);
	} else {
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	}

	glEnableVertexAttribArray(1);
	if (interleaved) {
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 3 + sizeof(float) * 2, (void*)(sizeof(float) * 3));
	} else {
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * vertexCount));
	}
	glBindVertexArray(0);
}

void Mesh::Draw(const unsigned programId, const unsigned textureId) const
{
	glUseProgram(programId);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glUniform1i(glGetUniformLocation(programId, "diffuse"), 0);

	glBindVertexArray(vao);
	size_t verticesToPrint = (indexCount == 0) ? vertexCount : indexCount;
	glDrawElements(GL_TRIANGLES, verticesToPrint, GL_UNSIGNED_INT, nullptr);

	if (interleavedVertices) {
		RenderInterleaved(programId);
	}
	else {
		RenderSeparated(programId);
	}
}
