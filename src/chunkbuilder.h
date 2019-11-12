#ifndef CHUNKBUILDER_H
#define CHUNKBUILDER_H

#include <Spatial.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <MeshInstance.hpp>
#include <StaticBody.hpp>
#include <ConcavePolygonShape.hpp>
#include <SurfaceTool.hpp>
#include <ArrayMesh.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include "constants.h"
#include "chunk.h"
#include "meshbuilder.h"
#include "objectpool.h"

namespace godot {
	class ChunkBuilder {

	private :
		static ObjectPool<float, MAX_VERTICES_SIZE>& getVerticesPool() {
			static ObjectPool<float, MAX_VERTICES_SIZE> pool;
			return pool;
		};
		boost::asio::io_service ioService;
		boost::asio::io_service::work work;
		boost::thread_group threadpool;
	
		/*static Ref<SpatialMaterial> terrainMaterial;
		static Ref<Texture> terrainTexture;*/
	public:
		class Worker {

		private:
			MeshBuilder meshBuilder;

		public:
			void run(Chunk* chunk, Node* game);
		};

		ChunkBuilder() : ChunkBuilder(POOL_SIZE) {};
		explicit ChunkBuilder(size_t size) : work(ioService) {
			/*terrainMaterial = Ref<SpatialMaterial>(SpatialMaterial::_new());
			ResourceLoader* resourceLoader = ResourceLoader::get_singleton();
			terrainTexture = resourceLoader->load(String("res://Images/grass.png"));
			terrainMaterial->set_texture(SpatialMaterial::TEXTURE_ALBEDO, terrainTexture);*/

			for (size_t i = 0; i < size; ++i) {
				threadpool.create_thread(
					boost::bind(&boost::asio::io_service::run, &ioService));
			}
		}
		~ChunkBuilder() {
			ioService.stop();
			threadpool.interrupt_all();
		};

		void build(Chunk *chunk, Node *game);
	};

}

#endif
