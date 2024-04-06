<template>

<div class="data-view">

<DataList />

</div>

</template>

<script>
// @ is an alias to /src
import DataList from '@/components/DataList.vue';

export default {
  name: 'DataListView',
  props: {
  },
  data: function() {
    return {
    }
  },
  components: {
    DataList
  }
}
</script>

<style>

</style>