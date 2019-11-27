#ifndef CHUNKBUILDER_H
#define CHUNKBUILDER_H

#include <Spatial.hpp>
#include <String.hpp>
#include <Array.hpp>
#include <MeshInstance.hpp>
#include <StaticBody.hpp>
#include <CollisionShape.hpp>
#include <ConcavePolygonShape.hpp>
#include <SurfaceTool.hpp>
#include <ArrayMesh.hpp>

#include <iostream>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/future.hpp>

#include "constants.h"
#include "chunk.h"
#include "meshbuilder.h"
#include "objectpool.h"

using namespace std;
namespace bpt = boost::posix_time;

namespace godot {

	class ChunkBuilder {

	private :
		class Worker {
		private:
			static ObjectPool<float, MAX_VERTICES_SIZE, POOL_SIZE * 4>& getVerticesPool() {
				static ObjectPool<float, MAX_VERTICES_SIZE, POOL_SIZE * 4> pool;
				return pool;
			};
			MeshBuilder meshBuilder;
		public:
			void run(Chunk* chunk, Node* game);
		};

		boost::asio::io_service ioService;
		boost::asio::io_service::work work;
		boost::thread_group threadpool;
		
		/*static Ref<SpatialMaterial> terrainMaterial;
		static Ref<Texture> terrainTexture;*/
	public:
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
			threadpool.join_all();
		};
		void build(Chunk *chunk, Node* game);
	};

}

#endif
