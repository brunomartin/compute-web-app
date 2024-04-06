<template>

  <div>

    <b-button v-b-modal="'add-process-modal-'+data.item.id" variant="outline-info" style="border:none">
      <b-icon-plus-circle-fill font-scale="1.5"></b-icon-plus-circle-fill>
    </b-button>

    <b-modal
      :id="'add-process-modal-'+data.item.id"
      @show="retrieveProcessDefinitions(data.item)"
      @ok="addProcess(data.item.id, processDefinitionSelected)"
      @keydown.native.enter="addProcess(data.item.id, processDefinitionSelected)"
      :title="'Post a new process to data '+data.item.id"
      :ok-disabled="processDefinitionSelected == null"
      >

      <b-form-select v-model="processDefinitionSelected" :options="processDefinitions"></b-form-select>

      <template v-if="processDefinitionSelected">

        <div v-for="parameter in processDefinitionSelected.parameters" :key="parameter.name">

          <label :for="'type-'+parameter.name">{{parameter.name}}</label>
          <b-form-input :id="'type-'+parameter.name" :type="getType(parameter.type)" v-model="parameter.value">
          </b-form-input>
        </div>

      </template>

    </b-modal>

  </div>

</template>

<script>
// import Data from '@/components/Data.vue';
import axios from 'axios';

export default {
  name: 'DataList',
  props: {
    data: Object
  },
  data: function() {
    return {
      processDefinitionSelected: null,
      processDefinitions: null
    }
  },
  methods: {
    getType(type) {
      switch(type) {
        case 'string': return 'text'
        case 'integer':
        case 'number': return 'number'
        default: alert('Unknwon type '+type); return ''
      }
    },
    retrieveProcessDefinitions(data) {
      axios
        .get(process.env.VUE_APP_CWA_API_URL + '/process_definition')
        .then( (response) => {
          let items = response.data

          this.processDefinitions = []
          for(let index in items) {
            let item = items[index]

            let definition = {
              text: item.id,
              value: item
            }

            let add_definition = true

            // if not uploaded, add definition only has corresponing option
            if(!data.status.stored) {
              add_definition &= item.options && item.options.includes('early_process')
            }

            if(add_definition) {
              this.processDefinitions.push(definition)         
            }
          }

          for(let index in this.processDefinitions) {
            let definition = this.processDefinitions[index]

            for(let parameter_index in definition.value.parameters) {
              let parameter = definition.value.parameters[parameter_index]
              parameter.name = parameter_index
              if(parameter.default) {
                parameter.value = parameter.default
              } else {
                parameter.value = ''
              }
            }
          }

          if(this.processDefinitions.length > 0) {
            this.processDefinitionSelected = this.processDefinitions[0].value
          } else {
            this.processDefinitionSelected = null
          }
        })
        .catch(error => {
          console.log('error on store');
                      throw error;
        });
    },
    addProcess(data_id, process_definition, auto_start = true) {

      // build parameter list
      let parameters = {}
      for(let index in process_definition.parameters) {
        parameters[index] = process_definition.parameters[index].value
      }

      let data = {
        version: '1.0',
        data_id: data_id,
        process_definition_id: process_definition.id,
        parameters: parameters
      }

      axios
        .post(process.env.VUE_APP_CWA_API_URL + '/process', data)
        .then( (response) => {
          // console.log('response: ' + JSON.stringify(response));
          
          if(auto_start) {
            this.startProcess(response.data.process_id)
          }

        })
        .catch(error => {
          console.log('error on store');
                      throw error;
        });
    },
    startProcess(process_id) {

      let data = {
        version: '1.0',
        process_id: process_id,
        name: 'start'
      }

      axios
        .post(process.env.VUE_APP_CWA_API_URL + '/process_action', data)
        .then( (response) => {
          response
          // console.log('response: ' + JSON.stringify(response));
        })
        .catch(error => {
          console.log('error on store');
                      throw error;
        });
    }
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
</style>
