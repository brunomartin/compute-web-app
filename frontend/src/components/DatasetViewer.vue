<template>

  <div>

    <b-row>
      <b-col>

        <label for="image_group">Group : {{imageGroup}}/{{data.groups-1}}</label>

        <b-form-input
          id="image_group"
          v-model="imageGroup"
          type="range"
          @input="updateImageSrc()"
          min=0
          :max="data.groups-1"
        ></b-form-input>

        <div v-if="dataset.shape[2] > 1">
          <label for="image_group">Depth : {{imageDepth}}/{{dataset.shape[2]-1}}</label>

          <b-form-input
            
            id="image_depth"
            v-model="imageDepth"
            type="range"
            @input="updateImageSrc()"
            min=0
            :max="dataset.shape[2]-1"
          ></b-form-input>
        </div>

      </b-col>

      <b-col>
        <b-img :src="this.imageSrc" fluid alt="Fluid image"></b-img>
      </b-col>

    </b-row>

  </div>

</template>

<script>

export default {
  name: 'DatasetViewer',
  props: {
    type: String,
    data: Object,
    datasetName: String
  },
  computed: {
    
  },
  data: function() {
    return {
      dataset: null,
      imageSrc: null,
      imageGroup: 0,
      imageDepth: 0
    }
  },
  methods: {
    updateImageSrc() {
      let imageSrc = process.env.VUE_APP_CWA_API_URL
      imageSrc += '/' + this.type + '/' +  this.data.id
      imageSrc += '/group/' + this.imageGroup
      imageSrc += '/dataset/' + this.datasetName + '/image'
      imageSrc += '?start=' + this.imageDepth + '&length=' + 1
      this.imageSrc = imageSrc
    }
  },
  created() {
    // find target dataset and set image max depth
    this.dataset = this.data.datasets.find(x => x.name === this.datasetName)

    // if dataset has two dimensions, add third one set to 1
    if(this.dataset.shape.length == 2) {
      this.dataset.shape.push(1)
    }

    this.updateImageSrc()
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
</style>
