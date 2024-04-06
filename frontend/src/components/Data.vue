<template>

  <div>
  <b-row v-if="dataId==-1" class="data-header border-top border-left border-right border-dark">
    <b-col>Id</b-col>
    <b-col>Filename</b-col>
    <b-col>dataset count</b-col>
    <b-col>Add process</b-col>
  </b-row>

  <b-row v-else class="data border-top border-dark">
    <b-col>{{dataId}} <b-button v-b-toggle="'collapse-'+dataId" class="m-1"><b-icon-arrow-bar-bottom></b-icon-arrow-bar-bottom></b-button> </b-col>
    <b-col>{{filename}}</b-col>
    <b-col>{{datasetCount}}</b-col>
    <b-col>
      <b-button variant="light" v-on:click="onClick()"><b-icon-plus></b-icon-plus>
      </b-button>
    </b-col>
  </b-row>

  <b-collapse v-if="dataId>-1" :id="'collapse-'+dataId" accordion="data-accordion">
    <b-row>
      <b-col><DataImage :fileId="dataId"/></b-col>
    </b-row>
  </b-collapse>

  </div>

</template>

<script>
import DataImage from '@/components/DataImage.vue';
import axios from 'axios';

export default {
  name: 'Data',
  props: {
    dataId: Number
  },
  components: {
    DataImage
  },
  data: function() {
    return {
      info: null,
      filename: null,
      datasetCount: null
    }
  },
  methods: {
    onClick() {
      // alert("test" + JSON.stringify(this.info));
    }
  },
  mounted() {

    if(this.dataId < 0) {
      return;
    }

    axios
      .get(process.env.VUE_APP_CWA_API_URL + '/file/'+this.dataId)
      .then(response => {
        this.info = response.data;
        this.filename = this.info.filename;
        this.datasetCount = this.info.datasets.length;
      })
  },
  computed: {
    options() {
      return {
        ['v-b-toggle.collapse-'+this.dataId]: "whatever"
      }
    }
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
</style>
