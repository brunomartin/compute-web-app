<template>

  <div class="data-list">

    <loading :active.sync="isLoading" :can-cancel="false" :is-full-page="true"></loading>

    <b-form-checkbox v-model="periodicRetrieve">Watch Datas</b-form-checkbox>

    <b-table small striped show-empty bordered stacked="sm" :items="items" :fields="fields" :sort-by.sync="sortBy" :sort-desc.sync="sortDesc">

      <template v-slot:cell(size)="data">
        <span :id="'tooltip-size-'+data.item.id">{{(data.value/1024/1024).toFixed(2)}}</span>

        <b-tooltip :target="'tooltip-size-'+data.item.id" triggers="hover">
          {{ data.value }} Bytes
        </b-tooltip>
      </template>

      <template v-slot:cell(uploaded_bytes)="data">
        <span :id="'tooltip-uploaded_bytes-'+data.item.id">{{(data.value/1024/1024).toFixed(2)}}</span>

        <b-tooltip :target="'tooltip-uploaded_bytes-'+data.item.id" triggers="hover">
          {{ data.value }} Bytes
        </b-tooltip>
      </template>

      <template v-slot:cell(stored_bytes)="data">
        <span :id="'tooltip-stored_bytes-'+data.item.id">{{(data.value/1024/1024).toFixed(2)}}</span>

        <b-tooltip :target="'tooltip-stored_bytes-'+data.item.id" triggers="hover">
          {{ data.value }} Bytes
        </b-tooltip>
      </template>

      <template v-slot:cell(name)="data">
        <span :id="'tooltip-name-'+data.item.id">{{ data.value.slice(0, 16) + '...' }}</span>

        <b-tooltip :target="'tooltip-name-'+data.item.id" triggers="hover">
          {{ data.value }}
        </b-tooltip>
      </template>

      <template v-slot:cell(status)="data" style="text-align: center">
        <DataStatus :data="data"/>
      </template>

      <template v-slot:cell(info)="data">
        <DataInfo :data="data"/>
      </template>

      <template v-slot:cell(process)="data">
        <DataAddProcess :data="data"/>
      </template>

    </b-table>

  </div>

</template>

<script>
// import Data from '@/components/Data.vue';
import axios from 'axios';

import Loading from 'vue-loading-overlay'
import 'vue-loading-overlay/dist/vue-loading.css'

import DataStatus from '@/components/DataStatus.vue';
import DataAddProcess from '@/components/DataAddProcess.vue';
import DataInfo from '@/components/DataInfo.vue';

export default {
  name: 'DataList',
  props: {
  },
  data: function() {
    return {
      items: [],
      fields: [
        {key: 'id', label:'Data Id', sortable: true},
        {key: 'post_time', label:'Post Time', sortable: true, formatter: "formatDateAssigned"},
        {key: 'stored_bytes', label:'Size (MB)', sortable: true},
        {key: 'version', label:'Version', sortable: true},
        {key: 'status', label:'Upload Status', sortable: true},
        {key: 'info', label:'Info'},
        {key: 'process', label:'Process'},
      ],
      sortBy: 'post_time',
      sortDesc: true,
      isLoading: false,
      processDefinitionSelected: null,
      processDefinitions: [],
      uploadingDataIds: [],
      interval: null,
      periodicRetrieve: true
    }
  },
  filters: {
    
  },
  methods: {
    formatDateAssigned(value) {
      
      // check if date is valid, return null string if not
      if(!(value instanceof Date && !isNaN(value))) {
        return ''
      }

      var options = { year: '2-digit', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit' };
      return value.toLocaleDateString(undefined, options);
      // return value.toLocaleDateString('en-EN', options);
    },
    retrieveDatas() {
      axios
        .get(process.env.VUE_APP_CWA_API_URL + '/data?details')
        .then( (response) => {
          let items = response.data

          // parse item dates
          for(const index in items) {
            let item = items[index]
            item.post_time = new Date(item.post_time)
            item.upload_end_time = new Date(item.upload_end_time)
            item.store_end_time = new Date(item.store_end_time)

            if(!item.status.stored) {
              setTimeout(() => this.retrieveData(item.id), 1000)
            }
          }

          this.items = items

          this.isLoading = false
        })
        .catch(error => {
          console.log('error on store');
                      throw error;
        });
    },
    retrieveData(data_id) {
      axios
        .get(process.env.VUE_APP_CWA_API_URL + '/data/' + data_id)
        .then( (response) => {

          // find target item
          let item = this.items.find(x => x.id === data_id)

          // if not found, add it
          if(!item) {
            item = response.data
            this.items.push(item)
          }
          
          // assign response content to found item
          item = Object.assign(item, response.data);

          // convert datetimes
          item.post_time = new Date(item.post_time)
          item.upload_end_time = new Date(item.upload_end_time)
          item.store_end_time = new Date(item.store_end_time)

          if(!item.status.stored) {
            setTimeout(() => this.retrieveData(data_id), 1000)
          } else {
            const index = this.uploadingDataIds.indexOf(data_id);
            if (index > -1) {
              this.uploadingDataIds.splice(data_id, 1);
            }
          }
          
        })
        .catch(error => {
          console.log('error on store');
                      throw error;
        });

    },
    retrieveUploadingDatas() {
      axios
        .get(process.env.VUE_APP_CWA_API_URL + '/data_uploading')
        .then( (response) => {
          let data_ids = response.data

          for(let index in data_ids) {
            let data_id = data_ids[index]

            if(!this.uploadingDataIds.includes(data_id)) {
              console.log('new uploading data_id : ' + data_id)
              this.uploadingDataIds.push(data_id)
              this.retrieveData(data_id)
            }
          }

        })
        .catch(error => {
          console.log('error on store');
                      throw error;
        });
    }
  },
  components: {
    Loading,
    DataStatus,
    DataAddProcess,
    DataInfo
  },
  mounted() {
    this.isLoading = true

    // retrieve data, once done, this method will set isLoading to false
    this.retrieveDatas()

    // start periodic retrieve if default set to true
    if(this.periodicRetrieve && !this.interval) {
      this.interval = setInterval(() => this.retrieveUploadingDatas(), 1000)
    }
  },
  beforeDestroy () {
    clearInterval(this.interval)
  }, watch: {
    periodicRetrieve: function(value) {
      if(value) {
        if(!this.interval) {
          this.interval = setInterval(() => this.retrieveUploadingDatas(), 1000)
        }
      } else {
        clearInterval(this.interval)
        this.interval = null
      }
    }
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
</style>
