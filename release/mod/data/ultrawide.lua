-- FTL Ultrawide: presentation only. Ships, weapons and combat statistics retain
-- their original game data. Gameplay backgrounds use complete generated scenes;
-- the native center crop and Lua margins come from the same panorama. Title and
-- hangar retain their previously selected artwork.
local enabled, width = false, 1280
local config = io.open('ultrawide-runtime.cfg', 'r')
if config then
    for line in config:lines() do
        local key, value = line:match('^(%w+)=(%d+)$')
        if key == 'enabled' then enabled = value == '1' end
        if key == 'width' then width = tonumber(value) end
    end
    config:close()
end
if not enabled then return end
local padding = (width - 1280) / 2
local white = Graphics.GL_Color(1, 1, 1, 1)
local resource = Hyperspace.Resources or Hyperspace.resources
local sourceToPanorama = {
    -- BEGIN GENERATED BACKGROUND MAP
    ['stars/bg_blueStarcluster.png'] = {image='ultrawide/bg_blueStarcluster-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/bg_darknebula.png'] = {image='ultrawide/bg_darknebula-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/bg_dullstars.png'] = {image='ultrawide/bg_dullstars-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/bg_dullstars2.png'] = {image='ultrawide/bg_dullstars2-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/bg_lonelyBlueStar.png'] = {image='ultrawide/bg_lonelyBlueStar-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/bg_lonelyRedStar.png'] = {image='ultrawide/bg_lonelyRedStar-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/bg_lonelystar.png'] = {image='ultrawide/bg_lonelystar-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/bg_smallbluenebula.png'] = {image='ultrawide/bg_smallbluenebula-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['customizeUI/custom_background.png'] = {image='ultrawide/hangar-native-rails-v4.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula1.png'] = {image='ultrawide/z_bg_nebula1-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula2.png'] = {image='ultrawide/z_bg_nebula2-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula3.png'] = {image='ultrawide/z_bg_nebula3-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula4.png'] = {image='ultrawide/z_bg_nebula4-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula5.png'] = {image='ultrawide/z_bg_nebula5-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula6.png'] = {image='ultrawide/z_bg_nebula6-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula7.png'] = {image='ultrawide/z_bg_nebula7-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula8.png'] = {image='ultrawide/z_bg_nebula8-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula9.png'] = {image='ultrawide/z_bg_nebula9-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula10.png'] = {image='ultrawide/z_bg_nebula10-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula12.png'] = {image='ultrawide/z_bg_nebula12-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula13.png'] = {image='ultrawide/z_bg_nebula13-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_nebula16.png'] = {image='ultrawide/z_bg_nebula16-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_hubble_29.png'] = {image='ultrawide/z_bg_hubble_29-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_hubble_31.png'] = {image='ultrawide/z_bg_hubble_31-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_xandra_01.png'] = {image='ultrawide/z_bg_xandra_01-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_stars1.png'] = {image='ultrawide/z_bg_stars1-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/z_bg_sun_stars2.png'] = {image='ultrawide/z_bg_sun_stars2-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/low_nebula.png'] = {image='ultrawide/low_nebula-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/low_storm.png'] = {image='ultrawide/low_storm-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/low_asteroid.png'] = {image='ultrawide/low_asteroid-full-v3.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/low_pulsar.png'] = {image='ultrawide/low_pulsar-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['stars/low_sun.png'] = {image='ultrawide/low_sun-full-v1.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    ['main_menus/main_base2.png'] = {image='ultrawide/title-under-flare-v12.png', bounds={-220.0000,0.0000,1720.0000,720.0000}},
    -- END GENERATED BACKGROUND MAP
}
local panoramas, byTextureId = {}, {}
local initialized = false
local function initialize()
    if initialized then return end
    initialized = true
    for source, panorama in pairs(sourceToPanorama) do
        local original = resource:GetImageId(source)
        panorama.texture = resource:GetImageId(panorama.image)
        panorama.original = original
        panoramas[source] = panorama
        if original.id ~= 0 then byTextureId[original.id] = panorama end
    end
end
local function drawPanorama(panorama, color)
    if not panorama then return end
    local bounds = panorama.bounds
    local function clamp(value) return math.max(0.00001, math.min(0.99999, value)) end
    local function margin(x, span)
        Graphics.CSurface.GL_BlitImagePartial(panorama.texture, x, 0, span, 720,
            clamp((x - bounds[1]) / bounds[3]),
            clamp((x + span - bounds[1]) / bounds[3]),
            clamp(-bounds[2] / bounds[4]), clamp((720 - bounds[2]) / bounds[4]),
            1, color or white, false)
    end
    -- This routine draws only the margins around the native center rectangle.
    -- Whole-scene replacements stage that center from the same image. This
    -- also preserves sources containing alpha, such as hazard overlays.
    margin(-padding, padding)
    margin(1280, padding)
    -- No edge reflection or color strip is used. The margins must meet the
    -- native crop through the exact original panorama pixels.
end

local lastBackgroundId = nil
local function backgroundPanorama(texture)
    if not texture then return nil end
    -- SpaceManager.currentBack is already a GL_Texture*, not an ImageDesc.
    -- Refresh IDs after lazy loading instead of retaining startup zero IDs.
    local id = texture.id
    if id == 0 then return nil end
    if id ~= lastBackgroundId then
        for source, panorama in pairs(panoramas) do
            if panorama.original.id == id and id ~= 0 then
                byTextureId[id] = panorama
                break
            end
        end
        lastBackgroundId = id
    end
    return byTextureId[id]
end

script.on_render_event(Defines.RenderEvents.MAIN_MENU,
    function()
        initialize()
        local menu = Hyperspace.Global.GetInstance():GetCApp().menu
        if menu.shipBuilder.bOpen then
            -- Hyperspace replaces ShipBuilder::OnRender and applies its own
            -- quarter-brightness tint when its custom ship list is open.
            local shipList = Hyperspace.CustomShipSelect.GetInstance():IsOpen()
            drawPanorama(panoramas['customizeUI/custom_background.png'],
                shipList and Graphics.GL_Color(.25, .25, .25, 1) or white)
        else
            drawPanorama(panoramas['main_menus/main_base2.png'])
        end
    end,
    function() end)

-- Resume regression was isolated to the upstream Windows logger. World
-- callbacks run with the guarded logger fix and production profile inheritance.
local worldCallbacksEnabled = true
if worldCallbacksEnabled then
script.on_render_event(Defines.RenderEvents.LAYER_BACKGROUND,
    function()
        initialize()
        Graphics.CSurface.GL_PushMatrix()
        local space = Hyperspace.Global.GetInstance():GetCApp().world.space
        if space.bNebula and not Hyperspace.Settings.lowend then
            -- Native animated clouds replace the space backdrop in this branch.
            -- Map that entire decorative layer, including lightning, onto the
            -- wide canvas. Do not leave unrelated starfield art in the margins.
            Graphics.CSurface.GL_DrawRect(0, 0, width, 720, Graphics.GL_Color(0,0,0,1))
            Graphics.CSurface.GL_Scale(width / 1280, 1, 1)
            return
        end
        Graphics.CSurface.GL_Translate(padding, 0, 0)
        local health = 1
        local player = Hyperspace.ships.player
        if player then
            local hull = player.ship.hullIntegrity
            if hull.second > 0 then health = hull.first / hull.second end
        end
        -- Match SpaceManager::OnRenderBackground's native pause/low-hull grade.
        -- Its red and green/blue channels differ; a black veil cannot reproduce it.
        local red, green = 1, 1 - .5 * (1-health)^2
        if space.gamePaused then red, green = .4 + .4*(1-health), .4 end
        local panorama
        if space.bNebula then
            panorama = panoramas[space.bStorm and 'stars/low_storm.png' or 'stars/low_nebula.png']
        else
            panorama = backgroundPanorama(space.currentBack)
        end
        drawPanorama(panorama, Graphics.GL_Color(red, green, green, 1))
    end,
    function() Graphics.CSurface.GL_PopMatrix() end)

script.on_render_event(Defines.RenderEvents.LAYER_FOREGROUND,
    function()
        Graphics.CSurface.GL_PushMatrix()
        Graphics.CSurface.GL_Translate(padding, 0, 0)
    end,
    function() Graphics.CSurface.GL_PopMatrix() end)

script.on_internal_event(Defines.InternalEvents.ON_TICK, function()
    local app = Hyperspace.Global.GetInstance():GetCApp()
    if app.gui then app.gui.pauseTextLoc.x = math.floor(width / 2) end
end)
end
