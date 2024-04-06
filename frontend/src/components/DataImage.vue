<template>

  <div class="image">
  <div>
    <img :src="imageUrl">
  </div>

  <div>
    <b-row>
      <b-col>
      Dataset : {{dataset.value}}
      </b-col>
      <b-col>
        <b-form-slider class="dataset-slider"
          :value="dataset.value"
          :min="0" :max="dataset.max" :step="1"
          @change="datasetSlideChange" @slide-start="slideStart" @slide-stop="slideStop">
        </b-form-slider>
      </b-col>
    </b-row>
    
    <b-row>
      <b-col>
      Echo : {{echo.value}}
      </b-col>
      <b-col>
        <b-form-slider class="echo-slider"
          :value="echo.value"
          :min="0" :max="echo.max" :step="1"
          @change="echoSlideChange" @slide-start="slideStart" @slide-stop="slideStop">
        </b-form-slider>
      </b-col>
    </b-row>
  </div>

  </div>

</template>

<script>
import axios from 'axios';

export default {
  name: 'DataImage',
  props: {
    fileId: {
      type: Number,
      default: 0
    }
  },
  data: function() {
    return {
      imageUrl: '',
      value: 0,
      max: 10,
      dataset: {
        value: 0,
        max: 10,
      },
      echo: {
        value: 0,
        max: 10,
      },
      shape: []
    }
  },
  methods: {
    updataImageUrl() {
      this.imageUrl = process.env.VUE_APP_CWA_API_URL + '/file/'
      this.imageUrl += this.fileId + '/dataset/'
      this.imageUrl += this.dataset.value + '/image?start='
      this.imageUrl += this.echo.value + '&length=1'
    },
    slideStart () {
      console.log('slideStarted')
    },
    slideStop () {
      console.log('slideStopped')
    },
    datasetSlideChange (value) {
      console.log('datasetSlideChange' + value.oldValue + ' => ' + value.newValue)
      this.dataset.value = value.newValue;
      this.updataImageUrl()
    },
    echoSlideChange (value) {
      console.log('echoSlideChange' + value.oldValue + ' => ' + value.newValue)
      this.echo.value = value.newValue;
      this.updataImageUrl()
    }
  },
  mounted() {
    let value = {"newValue": 0};
    this.datasetSlideChange(value);
    this.echoSlideChange(value);

    axios
      .get(process.env.VUE_APP_CWA_API_URL + '/file/'+this.fileId)
      .then( (response) => {
        let info = response.data;
        this.dataset.max = info.datasets.length;
      });

    axios
      .get(process.env.VUE_APP_CWA_API_URL + '/file/'+this.fileId+'/dataset/1')
      .then( (response) => {
        let info = response.data;
        this.shape = info.shape;
        this.echo.max = this.shape[2]-1;
      });

  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
</style>
