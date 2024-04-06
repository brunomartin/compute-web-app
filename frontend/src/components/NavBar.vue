<template>

<div id="bnavbar">
  <b-navbar toggleable="sm" type="dark" variant="info">
    <b-navbar-brand href="#">Compute Web App UI</b-navbar-brand>

    <b-navbar-toggle target="nav-collapse"></b-navbar-toggle>

    <b-collapse id="nav-collapse" is-nav>
      <b-navbar-nav>
        <b-nav-item to="/">Home</b-nav-item>
        <b-nav-item to="/data-list">Data List</b-nav-item>
        <b-nav-item to="/process-list">Process List</b-nav-item>
      </b-navbar-nav>
      
    </b-collapse>
  </b-navbar>
</div>

</template>

<script>
export default {
  name: 'NavBar',
  props: {
  }
}
</script>

<style>

</style>