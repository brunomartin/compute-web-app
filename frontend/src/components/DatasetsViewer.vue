<template>

  <div>

    <b-row>
      <b-form-checkbox-group
        v-model="selected"
        :options="options"
        value-field="item"
        text-field="name"
        class="ml-4"
        aria-label="Individual dataset"
        @input="updateDatasetsImageSrc()"
      ></b-form-checkbox-group>
    </b-row>

    <b-row>
      <b-col col cols="6" lg="3" v-for="option in selected" :key="option.name">
        <b-img :src="option.imageSrc" fluid alt="Fluid image"></b-img>
        <p>{{option.name}}</p>
      </b-col>
    </b-row>

    <b-row>

      <label for="image_group">Group : {{imageGroup}}/{{data.groups-1}}</label>

      <b-form-input
        id="image_group"
        v-model="imageGroup"
        type="range"
        @input="updateDatasetsImageSrc()"
        min=0
        :max="data.groups-1"
      ></b-form-input>

      <label v-if="depth > 1" for="image_group">Depth : {{imageDepth}}/{{depth-1}}</label>

      <b-form-input v-if="depth > 1"          
        id="image_depth"
        v-model="imageDepth"
        type="range"
        @input="updateDatasetsImageSrc()"
        min=0
        :max="depth-1"
      ></b-form-input>

    </b-row>

  </div>

</template>

<script>

export default {
  name: 'DatasetsViewer',
  props: {
    type: String,
    data: Object,
    datasetNames: Array
  },
  computed: {
    
  },
  data: function() {
    return {
      options: [],
      selected: [],
      depth: 1,
      imageGroup: 0,
      imageDepth: 0
    }
  },
  methods: {
    updateDatasetsImageSrc() {
      for(let option of this.selected) {
        let imageSrc = process.env.VUE_APP_CWA_API_URL
        imageSrc += '/' + this.type + '/' +  this.data.id
        imageSrc += '/group/' + this.imageGroup
        imageSrc += '/dataset/' + option.name + '/image'
        imageSrc += '?start=' + this.imageDepth + '&length=' + 1
        option.imageSrc = imageSrc
      }
    }
  },
  created() {
    // find target datasets
    for(const datasetName of this.datasetNames) {
      // find dataset with name
      let dataset = this.data.datasets.find(x => x.name === datasetName)

      // if dataset has two dimensions, add third one set to 1
      if(dataset.shape.length == 2) {
        dataset.shape.push(1)
      } else {
        if(dataset.shape[2] > this.depth) {
          this.depth = dataset.shape[2]
        }
      }

      this.options.push({
        item: dataset,
        name: dataset.name
      })

      // Select all by default
      this.selected.push(dataset)
    }
    this.updateDatasetsImageSrc()
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
</style>
