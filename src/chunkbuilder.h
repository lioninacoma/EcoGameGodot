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
#include <boost/thread/mutex.hpp>
#include <boost/pool/pool_alloc.hpp>

#include "constants.h"
#include "chunk.h"
#include "meshbuilder.h"

typedef boost::fast_pool_allocator<float[BUFFER_SIZE]> VerticesAllocator;
typedef boost::singleton_pool<boost::fast_pool_allocator_tag, sizeof(float[BUFFER_SIZE])> VerticesAllocatorPool;

namespace godot {

	class VerticesPool {
	private:
		boost::mutex mutex;
		queue<float*, list<float*, VerticesAllocator>> pool;
	public:
		static VerticesPool& get() { static VerticesPool pool; return pool; }

		VerticesPool() {
			for (int i = 0; i < 8; i++) {
				pool.push(new float[BUFFER_SIZE]);
			}
		};
		~VerticesPool() {
			VerticesAllocatorPool::purge_memory();
		};
		float* borrow() {
			mutex.lock();
			float* o = pool.front();
			pool.pop();
			mutex.unlock();
			return o;
		};
		void ret(float* o) {
			mutex.lock();
			pool.push(o);
			mutex.unlock();
		};
	};

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
