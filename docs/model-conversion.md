# Model Conversion Guide

Convert PyTorch ACE-Step checkpoints to GGUF format for use with ace-server, ace-lm, and ace-synth.

## Quick Start (Pre-Quantized Models)

If you just want to run inference, skip conversion entirely:

```bash
pip install huggingface_hub
./models.sh                   # Downloads ~7.7 GB of Q8_0 turbo models
./build/ace-server \
  --lm models/acestep-5Hz-lm-4B-Q8_0.gguf \
  --embedding models/Qwen3-Embedding-0.6B-Q8_0.gguf \
  --dit models/acestep-v15-turbo-Q8_0.gguf \
  --vae models/vae-BF16.gguf \
  --port 8080
```

`models.sh` options:

| Flag | Effect |
|------|--------|
| *(none)* | Q8_0 turbo essentials (~7.7 GB) |
| `--all` | Every model + every quant (~97 GB) |
| `--quant Q6_K` | Pick a specific quantization |
| `--lm 0.6B` | Use smaller LM (0.6B, 1.7B, or 4B) |
| `--sft` | Include SFT DiT variant |
| `--base` | Include base DiT variant |
| `--shifts` | Include shift1/shift3/continuous variants |

## Full Conversion Pipeline

For custom or fine-tuned models, follow these three steps:

```
checkpoints.sh → convert.py → quantize
(safetensors)    (BF16 GGUF)   (Q4-Q8 GGUF)
```

### Prerequisites

```bash
pip install gguf numpy huggingface_hub
```

Disk space: ~35 GB for default checkpoints, ~150 GB for all variants.

### Step 1: Download Raw Checkpoints

```bash
./checkpoints.sh              # Default: turbo + 4B LM + VAE + embedding
./checkpoints.sh --all        # All variants
```

This downloads safetensors files from HuggingFace into `checkpoints/`:

```
checkpoints/
  Qwen3-Embedding-0.6B/      # Text encoder
    model.safetensors, config.json, vocab.json, merges.txt
  acestep-5Hz-lm-4B/          # Language model (sharded)
    model.safetensors.index.json, model-00001-of-00003.safetensors, ...
    config.json, vocab.json, merges.txt
  acestep-v15-turbo/           # DiT model (sharded)
    model-00001-of-00005.safetensors, ...
    config.json, silence_latent.pt
  vae/                         # VAE decoder
    diffusion_pytorch_model.safetensors, config.json
```

### Step 2: Convert to GGUF

```bash
python3 convert.py
```

This auto-detects all models in `checkpoints/` and writes BF16 GGUF files to `models/`:

```
models/
  Qwen3-Embedding-0.6B-BF16.gguf
  acestep-5Hz-lm-4B-BF16.gguf
  acestep-v15-turbo-BF16.gguf
  vae-BF16.gguf
```

The converter embeds the full model config, BPE tokenizer (for LM and text-enc), and silence latent (for DiT) into the GGUF file.

### Step 3: Quantize

```bash
# Batch quantize all models with recommended settings:
./quantize.sh

# Or quantize individually:
./build/quantize models/acestep-v15-turbo-BF16.gguf models/acestep-v15-turbo-Q8_0.gguf Q8_0
```

Usage: `./build/quantize <input.gguf> <output.gguf> <type>`

## Quantization Types

From lowest to highest quality:

| Type | Description | Recommended For |
|------|-------------|-----------------|
| Q2_K | 2-bit with K-quant | Not recommended |
| Q3_K_S | 3-bit small | Extreme memory constraint |
| Q3_K_M | 3-bit medium | Low memory |
| Q3_K_L | 3-bit large | Low memory, better quality |
| Q4_K_S | 4-bit small | Good balance for DiT |
| **Q4_K_M** | **4-bit medium** | **DiT models (good quality/size)** |
| Q5_K_S | 5-bit small | Higher quality |
| **Q5_K_M** | **5-bit medium** | **LM-4B (recommended)** |
| **Q6_K** | 6-bit | High quality |
| **Q8_0** | 8-bit | **Best quality, default** |

## Per-Model Quantization Rules

Not all models support all quantization levels:

| Model | Supported Quants | Notes |
|-------|-----------------|-------|
| **VAE** | BF16 only | Never quantize — quality-critical |
| **Embedding** (0.6B) | BF16, Q8_0 | Too small for aggressive quant |
| **LM** (0.6B, 1.7B) | BF16, Q8_0 | Too small for aggressive quant |
| **LM** (4B) | BF16, Q5_K_M, Q6_K, Q8_0 | **Do NOT use Q4_K_M** — breaks audio code generation |
| **DiT** (all variants) | BF16, Q4_K_M, Q5_K_M, Q6_K, Q8_0 | Full range supported |

**Critical warning:** Q4_K_M on the 4B LM produces broken audio codes. Use Q5_K_M or higher.

## Converting Custom / Fine-Tuned Models

If you fine-tuned a model using ACE-Step-1.5 LoRA training:

1. **Merge LoRA into base model** (using the ACE-Step-1.5 Python tools)
2. **Copy the merged safetensors + config.json** into `checkpoints/<your-model-name>/`
3. **Name convention matters** — the converter classifies models by prefix:
   - `acestep-5Hz-lm*` → Language Model
   - `acestep-v15*` → DiT
   - `Qwen3-Embedding*` → Text Encoder
   - `vae` → VAE
4. Run `python3 convert.py` — it will auto-detect your model
5. Quantize as needed

For models with non-standard names, rename the directory to match the expected prefix.

## Verification

After conversion, verify the model loads correctly:

```bash
# Check LM
./build/ace-lm --lm models/your-model-Q8_0.gguf 2>&1 | head -10

# Check DiT + VAE
./build/ace-synth \
  --dit models/your-dit-Q8_0.gguf \
  --vae models/vae-BF16.gguf 2>&1 | head -10

# Full pipeline test
./build/ace-server \
  --lm models/acestep-5Hz-lm-4B-Q8_0.gguf \
  --embedding models/Qwen3-Embedding-0.6B-Q8_0.gguf \
  --dit models/your-dit-Q8_0.gguf \
  --vae models/vae-BF16.gguf \
  --port 8080
```

Then test generation:

```bash
curl -s http://localhost:8080/props
# Should return: {"status":{"lm":"ok","synth":"ok"}}
```

## Troubleshooting

### "FATAL: build/quantize missing"
Build the project first: `mkdir build && cd build && cmake .. && cmake --build . --config Release`

### "No safetensors found"
Check that `checkpoints/<model>/` contains either `model.safetensors` or `model.safetensors.index.json`.

### "vocab.json not found"
Only LM and text-encoder models need tokenizer files. For DiT and VAE this warning is normal.

### Quantized model produces silence or noise
- VAE: Must be BF16 (never quantize)
- LM-4B: Avoid Q4_K_M (use Q5_K_M+)
- Try a higher quantization level (Q6_K or Q8_0)

### Out of disk space during conversion
BF16 GGUF files are roughly the same size as the source safetensors. Budget ~35 GB for default models, plus space for quantized variants.

## GGUF Architecture Reference

Each model type stores specific metadata in the GGUF file:

| Architecture | Key Metadata |
|-------------|-------------|
| `acestep-lm` | block_count, embedding_length, vocab_size, tie_word_embeddings, tokenizer |
| `acestep-dit` | block_count, audio_acoustic_hidden_dim, patch_size, fsq_dim, is_turbo, silence_latent |
| `acestep-text-enc` | block_count, embedding_length, vocab_size, tokenizer |
| `acestep-vae` | block_count, hidden_size |

All models store the full `config.json` as `acestep.config_json` in GGUF metadata.
