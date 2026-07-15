#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include <random>

#include <hj/ai/vector_index.hpp>

#include "llm.h"

class memory_mgr
{
  public:
    memory_mgr() {};
    ~memory_mgr() {};

    static memory_mgr &instance()
    {
        static memory_mgr inst;
        return inst;
    }

    static hj::vector_index<hj::vindex_flat_l2_t> create_index()
    {
        return hj::vector_index<hj::vindex_flat_l2_t>();
    }

    void distribution(std::vector<float> &dst,
                      std::mt19937 &rng = std::mt19937(std::random_device{}()));

    template <typename T>
    int build_index(hj::vector_index<T>        &index,
                    const std::string          &model,
                    const std::string          &chunk,
                    hj::llama::context_params_t ctx_params,
                    int                         dimension,
                    bool                        add_special,
                    bool                        parse_special)
    {
        // build index
        index.build(dimension);

        // format text for embedding
        std::string formatted_text =
            "Represent this sentence for searching relevant passages: " + chunk;

        // get embedding
        std::vector<float> embedding;
        embedding.resize(dimension, 0.0f);
        int ret = llm_mgr::instance().get_embedding(embedding,
                                                    model,
                                                    formatted_text,
                                                    ctx_params,
                                                    dimension,
                                                    add_special,
                                                    parse_special);
        if(ret != OK)
        {
            LOG_ERROR("Failed to get embedding, error code: {}", ret);
            return ret;
        }

        bool valid          = false;
        int  non_zero_count = 0;
        for(float v : embedding)
        {
            if(v != 0.0f)
            {
                valid = true;
                non_zero_count++;
                break;
            }
        }

        if(!valid)
        {
            LOG_ERROR("Failed to get valid embedding for chunk: {}, embedding "
                      "size: {}, all zeros",
                      chunk.substr(0, 50),
                      embedding.size());

            for(int i = 0; i < std::min(10, (int) embedding.size()); ++i)
                LOG_DEBUG("embedding[{}] = {}", i, embedding[i]);

            return LLM_ERR_EMBEDDING_INVALID;
        }

        LOG_DEBUG("Embedding validation passed, non-zero count: {}/{}",
                  non_zero_count,
                  embedding.size());

        index.add(1, embedding.data());
        LOG_DEBUG("Successfully added embedding to index, dimension: {}",
                  dimension);
        return OK;
    }
};

#endif // MEMORY_H