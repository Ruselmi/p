import { pipeline } from 'https://cdn.jsdelivr.net/npm/@xenova/transformers@2.17.2';

const cache = {};

async function createSentimentPipeline(mode = 'full') {
  const model = mode === 'full'
    ? 'Xenova/distilbert-base-uncased-finetuned-sst-2-english'
    : 'Xenova/distilbert-base-uncased-finetuned-sst-2-english';

  const key = `sentiment:${model}`;
  if (!cache[key]) {
    cache[key] = await pipeline('sentiment-analysis', model);
  }
  return cache[key];
}

window.transformersBridge = {
  pipeline,
  createSentimentPipeline,
  ready: true,
};
