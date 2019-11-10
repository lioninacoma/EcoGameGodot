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
//#include <SpatialMaterial.hpp>
//#include <Texture.hpp>
//#include <ResourceLoader.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/fiber/future/promise.hpp>

#include "constants.h"
#include "chunk.h"
#include "meshbuilder.h"

namespace godot {

	class ChunkBuilder {

	private :
		boost::asio::io_service ioService;
		boost::asio::io_service::work work;
		boost::thread_group threadpool;

		/*static Ref<SpatialMaterial> terrainMaterial;
		static Ref<Texture> terrainTexture;*/
	public:
		class Worker {

		private:
			MeshBuilder* meshBuilder = new MeshBuilder();

		public:
			void run(Chunk* chunk, Node* game);
		};

		ChunkBuilder() : ChunkBuilder(THREAD_POOL_SIZE) {};
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
			threadpool.join_all();
		};

		void build(Chunk *chunk, Node *game);
	};

}

#endif
